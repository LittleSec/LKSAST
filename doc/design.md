以 Linux 5.8 为例，defconfig，LLVM=1，共两千多个编译单元（TU），内存足够的情况下能一次性把两千多个 ast 文件（约6GB）load 进内存，然而在对每个 ast 文件进行简单的 RecursiveASTVisitor 就会报出 LLVM ERROR: out of memory 错误，然而此时该进程只是大约使用了 32G 内存，物理内存是足够的。该错误应该是 llvm 的 bug，否则应该是操作系统或者是 cpp 层面报错才对。

因此折中的方法是将一个 ast 文件的分析结果先存起来（例如存成文件），分析完后就 pop 出内存，再分析下一个，然后再将所有 ast 的分析结果汇总。若使用该方法，还有一个好处是分析每个 ast 是的独立的，可以并行进行。

----------

## 假定输入

1. 所有的 ast 文件及其绝对路径信息（astList.txt）
2. clang-extdef-mapping生成的记录Decl定义点的信息（clang-extdef-mapping.txt），用于提取 函数定义->所在文件 的映射关系，当然这个不完整，部分inline的定义点在头文件，同时也不是每个函数都需要分析
3. 源代码文件名->ast文件名 的映射信息（src2ast.txt），这个文件里的信息也是有可能会被更改的。初试的信息是提取compile_command.json的，然而有些需要分析的函数可能在头文件里就定义了，这样和上面2点的信息是一块更新的


## 一个 ast 文件的分析结果应当含

每个需要分析的函数所调用的函数，即每个需要分析的函数的第一层 callgraph（CG），其中由于函数调用由直接调用和通过函数指针调用，因此需要额外分析函数指针，这也是该项目的重点。同时可以顺便记录“内核资源对象”的访问情况。

### 关于 CG

直接调用比较简单，clang ast 能直接分析出。这里重点分析函数指针，根据目前的观察，linux kernel 中函数指针调用的情况有三种：
  1. 函数指针为结构体里的某个域(MemberExpr, FieldDecl)
  2. 函数指针是函数指针数组的某个值(ArraySubscriptExpr)
  3. 函数指针是通过参数传递进来的(ParmVarDecl)
>可能还有其他情况，暂时先忽略。

对于情况 1 和 2，由于某个结构体或者是数组在一个 TU 里可能不止出现一次调用点；指向信息也不止一个函数；同时某些指向信息没法在当前 TU 中分析完成 => 因此没必要在每个函数通过函数指针调用函数时，把这些指向都存储一遍，而是存一个类似于 tag 的信息，然后用类似于 table 的结构可以通过前述的 tag 去找到这函数指针可能指向的信息，具体来说，例如在 c++ 里，我们可以用 `std::map<tag, std::list<MayPointTo>>` 表示这个结构。

对于情况 3，我们需要把含有函数指针参数的函数记录下来，要把调用这些函数的点也要记录下来（假设这类函数都通过直接调用）。
1. 对于前者信息：用于分析该形参在函数体类用来干什么，假设用途只有两种，一是函数调用，而是赋值各某个结构体域(1)或函数指针数组(2)
2. 对于后者信息：用于分析该实参，和前者信息进行绑定，即为可能指向的函数。（这里只考虑实参是一个显示的函数，而不是函数指针）


### 关于“内核资源对象”的访问情况

当前先不指定哪些是内核资源访问对象，先把所有访问全局变量和结构体成员的情况记录下来。之后整合结果的时候再定义哪些属于“内核资源对象”（eg.根据频率来算）。记录的信息应当包括访问了哪些变量，r/w，必要时记录一下访问点？（eg.是否是判断条件）


### 关于需要分析的函数

暂定符合一下情况的函数不会出现在CG中(即不需要分析):

1. 含有 AsmStmt 的函数
2. **inline函数**中没有使用内核资源的函数(存疑)
    + 特别提出一种情况，假设函数fun1的定义里没有在语义层面上访问内核资源对象，那么即使调用fun1时传参传的是一个内核资源对象（可能以指针方式传？），fun1也算**不**需要分析的函数
3. 通过配置项指定某些路径或文件中的函数

当然这有个问题是第一遍逐个文件分析CG的时候，当分析到某个直接调用时，还无法知道callee是否为不需要分析的函数，因此这时仍然需要加入到CG中。之后在整合结果时再做判断和剔除。

### 其他

除了上述两点，其实还要顺便记录函数指针调用点的情况，上面也有所提及。参考关于CG 对于情况3


----------

## 一些实现上的细节和妥协

声明（declaration, decl）和定义（definition, def）是两个不同的概念，通常来说，decl 在头文件，而 def 在某个 c 源文件，而一个头文件通常会被很多个 c 源文件 include，多个 ast 可能会有同一个副本

----------

## 关于 script

虽然部分脚本有同样的代码，但不要轻易提取出来到一个文件，然后用引用的方式引入这些代码，因为这些**脚本不一定是在本目录运行**，一般来说需要复制到其他目录再运行。

```
main() {
  func1(fp); // => info(func1 0->fp)
}

func1(cb fp1) {
  call add(100,200); // => func1.cg.add("add")
  call struct fop.read() // => func1.cg.add("struct fop.read")
  call func2(fp1); // => func1.cg.add("func2")
                   // => info.add("func2 0->func1 0")
}

func2(cb fp1) {
  call fp1(100,200); // => func2.cg.add("func2 0")
  struct fop.write = fp1; // => info.add("struct fop.write->func2 0")
  call func3(fp1); // => func2.cg.add("func3")
                   // => info.add("func3 0->func2 0")
}

func3(cb fp1) {
  call fp1(100,200); // => func3.cg.add("func3 0")
}
```


## 对于lhs=rhs, 已知的无法处理的情况

### Record 不为 Struct 的情况
union里可能定义函数指针，例如`fs/proc/internal.h:85`的
```c
union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
	const char *lsm;
};
```
lhs此时也是MemberExpr，但是无法处理的
```c
struct {
  union {
    (*funcptr);
    int;
  };
};
```

### lhs 真的就是个纯正的函数指针变量
如mm/truncate.c:160
```c
void do_invalidatepage(struct page *page, unsigned int offset,
		       unsigned int length)
{
	void (*invalidatepage)(struct page *, unsigned int, unsigned int);

	invalidatepage = page->mapping->a_ops->invalidatepage;
#ifdef CONFIG_BLOCK
	if (!invalidatepage)
		invalidatepage = block_invalidatepage;
#endif
	if (invalidatepage)
		(*invalidatepage)(page, offset, length);
}
```
