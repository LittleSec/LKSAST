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

直接调用比较简单，clang ast 能直接分析出。这里重点分析函数指针，根据目前的观察，linux kernel 中函数指针调用的情况有四种：
  1. 函数指针为结构体里的某个域(MemberExpr, FieldDecl)
  2. 函数指针是函数指针数组的某个值(ArraySubscriptExpr)
  3. 函数指针是通过参数传递进来的(ParmVarDecl)
  4. 函数指针变量(VarDecl)(Global/Local)
>可能还有其他情况，暂时先忽略。
>
>例如返回类型是函数指针类型

对于情况 1 和 2，由于某个结构体或者是数组在一个 TU 里可能不止出现一次调用点；指向信息也不止一个函数；同时某些指向信息没法在当前 TU 中分析完成 => 因此没必要在每个函数通过函数指针调用函数时，把这些指向都存储一遍，而是存一个类似于 tag 的信息，然后用类似于 table 的结构可以通过前述的 tag 去找到这函数指针可能指向的信息，具体来说，例如在 c++ 里，我们可以用 `std::map<tag, std::list<MayPointTo>>` 表示这个结构。

对于情况 3，我们需要把含有函数指针参数的函数记录下来，要把调用这些函数的点也要记录下来（假设这类函数都通过直接调用）。
1. 对于前者信息：用于分析该形参在函数体类用来干什么，假设用途只有两种，一是函数调用，而是赋值各某个结构体域(1)或函数指针数组(2)
2. 对于后者信息：用于分析该实参，和前者信息进行绑定，即为可能指向的函数。（这里只考虑实参是一个显示的函数，而不是函数指针）

2/4情况可以合并，因为数组本质上也是一个普通的变量(DeclRefExpr)

### 关于“内核资源对象”的访问情况

当前先不指定哪些是内核资源访问对象，先把所有访问全局变量和结构体成员的情况记录下来。之后整合结果的时候再定义哪些属于“内核资源对象”（eg.根据频率来算）。记录的信息应当包括访问了哪些变量，r/w，必要时记录一下访问点？（eg.是否是判断条件）

有些结构体几乎每个syscall都会用到，而且访问类型都一样（读/写），这种通过后期整合完整的syscall所访问的资源时，调整整合逻辑或配置文件的忽略分析代码路径来过滤。

### 关于需要分析的函数

暂定符合一下情况的函数不会出现在CG中(即不需要分析):
>这些条件可能会产生漏报

1. 含有 AsmStmt 的函数(存疑)
    + 有些函数只是部分内联汇编代码（例如一些完全依赖指令架构的特殊的指令）来完成部分工作，但其他代码依然有分析的价值
    + 具体例如`include/asm-generic/atomic-instrumented.h`里有些函数是依赖于体系结构，直接用汇编完成，如1645行的`#define xchg(ptr, ...)`，其使用点如`kernel/rcu/tree.c:3201`，`kfree_call_rcu_add_ptr_to_bulk()`里看起来就没有明显的汇编，应该需要继续分析
    + 目前有 ASM_DEBUG 宏来控制，默认还是会分析，而不是跳过不分析
2. **inline函数**中没有使用内核资源的函数(存疑)
    + 特别提出一种情况，假设函数fun1的定义里没有在语义层面上访问内核资源对象，那么即使调用fun1时传参传的是一个内核资源对象（可能以指针方式传？），fun1也算**不**需要分析的函数
3. 通过配置项指定某些路径或文件中的函数

当然这有个问题是第一遍逐个文件分析CG的时候，当分析到某个直接调用时，还无法知道callee是否为不需要分析的函数，因此这时仍然需要加入到CG中。之后在整合结果时再做判断和剔除。

### 其他

除了上述两点，其实还要顺便记录函数指针调用点的情况，上面也有所提及。参考关于CG 对于情况3


----------

## 一些实现上的细节和妥协

声明（declaration, decl）和定义（definition, def）是两个不同的概念，通常来说，decl 在头文件，而 def 在某个 c 源文件，而一个头文件通常会被很多个 c 源文件 include，多个 ast 可能会有同一个副本

因此有专门的 map 来记录某个函数的分析结果对应哪个ast结果，即 fun2json

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

### Record 为 匿名（结构体或联合体）
```c
struct {
  union {
    (*funcptr);
    int;
  };
};
```

### lhs是函数指针类型，但是rhs不是指针类型
>目前这种情况的处理方法就是产生一个 NULLPtr 作为 stub
如linux-5.8/lib/sort.c:199里的
```
void sort_r(void *base, size_t num, size_t size,
	    cmp_r_func_t cmp_func,
	    swap_func_t swap_func,
	    const void *priv)
```
swap_func若为NULL，则会赋值为(swap_func_t)0、(swap_func_t)1、(swap_func_t)02

CStyleCastExpr 0x55bfc3e7c5c8 'swap_func_t':'void (*)(void *, void *, int)' <IntegralToPointer>
`-IntegerLiteral 0x55bfc3e7c5a8 'int' 2

而后在
```
static void do_swap(void *a, void *b, size_t size, swap_func_t swap_func)
```
根据swap_func的值调用不同的函数。

根据宏我们可以窥探这一设计
```
/*
 * The values are arbitrary as long as they can't be confused with
 * a pointer, but small integers make for the smallest compare
 * instructions.
 */
#define SWAP_WORDS_64 (swap_func_t)0
#define SWAP_WORDS_32 (swap_func_t)1
#define SWAP_BYTES    (swap_func_t)2
```

### 连续赋值符号：bpf_op = bpf_chk = ops->ndo_bpf;
[Err] lhs is ptr, but rhs not
/home/razzer/Downloads/linux-5.8.clang/net/core/dev.c:8814:19
BinaryOperator 0x55e6ab0c0860 'bpf_op_t':'int (*)(struct net_device *, struct netdev_bpf *)' '='
|-DeclRefExpr 0x55e6ab0c0840 'bpf_op_t':'int (*)(struct net_device *, struct netdev_bpf *)' lvalue Var 0x55e6ab0bec50 'bpf_chk' 'bpf_op_t':'int (*)(struct net_device *, struct netdev_bpf *)'
`-ImplicitCastExpr 0x55e6ab0c0828 'int (*)(struct net_device *, struct netdev_bpf *)' <LValueToRValue>
  `-MemberExpr 0x55e6ab0c07f8 'int (*const)(struct net_device *, struct netdev_bpf *)' lvalue ->ndo_bpf 0x55e6aaf38110
    `-ImplicitCastExpr 0x55e6ab0c07e0 'const struct net_device_ops *' <LValueToRValue>
      `-DeclRefExpr 0x55e6ab0c07c0 'const struct net_device_ops *' lvalue Var 0x55e6ab0bdf70 'ops' 'const struct net_device_ops *'


### rhs 是一个函数调用，即某个函数的返回类型是函数指针类型
/home/razzer/Downloads/linux-5.8.clang/kernel/trace/trace_events_filter.c:1373:15
CallExpr 0x556144886948 'filter_pred_fn_t':'int (*)(struct filter_pred *, void *)'
|-ImplicitCastExpr 0x5561448868d8 'filter_pred_fn_t (*)(enum filter_op_ids, int, int)' <FunctionToPointerDecay>
| `-DeclRefExpr 0x5561448868b8 'filter_pred_fn_t (enum filter_op_ids, int, int)' Function 0x5561445182b0 'select_comparison_fn' 'filter_pred_fn_t (enum filter_op_ids, int, int)'
|-ImplicitCastExpr 0x5561448868a0 'enum filter_op_ids':'enum filter_op_ids' <IntegralCast>
| `-ImplicitCastExpr 0x556144886888 'int' <LValueToRValue>
|   `-MemberExpr 0x556144886858 'int' lvalue ->op 0x556144838c08
|     `-ImplicitCastExpr 0x556144886840 'struct filter_pred *' <LValueToRValue>
|       `-DeclRefExpr 0x556144886820 'struct filter_pred *' lvalue Var 0x556144885fb8 'pred' 'struct filter_pred *'
|-ImplicitCastExpr 0x556144886808 'int' <LValueToRValue>
| `-MemberExpr 0x5561448867d8 'int' lvalue ->size 0x556144837ef8
|   `-ImplicitCastExpr 0x5561448867c0 'struct ftrace_event_field *' <LValueToRValue>
|     `-DeclRefExpr 0x5561448867a0 'struct ftrace_event_field *' lvalue Var 0x5561448866b0 'field' 'struct ftrace_event_field *'
`-ImplicitCastExpr 0x556144886788 'int' <LValueToRValue>
  `-MemberExpr 0x556144886758 'int' lvalue ->is_signed 0x556144837f68
    `-ImplicitCastExpr 0x556144886740 'struct ftrace_event_field *' <LValueToRValue>
      `-DeclRefExpr 0x556144886688 'struct ftrace_event_field *' lvalue Var 0x5561448866b0 'field' 'struct ftrace_event_field *'
[Err] lhs is ptr, but rhs not
/home/razzer/Downloads/linux-5.8.clang/fs/autofs/dev-ioctl.c:623:7
CallExpr 0x5561b0bb7fa8 'ioctl_fn':'int (*)(struct file *, struct autofs_sb_info *, struct autofs_dev_ioctl *)'
|-ImplicitCastExpr 0x5561b0bb7f30 'ioctl_fn (*)(unsigned int)' <FunctionToPointerDecay>
| `-DeclRefExpr 0x5561b0bb7f10 'ioctl_fn (unsigned int)' Function 0x5561b0862898 'lookup_dev_ioctl' 'ioctl_fn (unsigned int)'
`-ImplicitCastExpr 0x5561b0bb7ef8 'unsigned int' <LValueToRValue>
  `-DeclRefExpr 0x5561b0bb7ed8 'unsigned int' lvalue Var 0x5561b0ba7860 'cmd' 'unsigned int'

### lhs 是 ArraySubscriptExpr-MemberExpr 混合
[Err] rhs is ptr, but lhs not
/home/razzer/Downloads/linux-5.8.clang/sound/pci/hda/hda_intel.c:695:4
ArraySubscriptExpr 0x556184b67980 'azx_get_pos_callback_t':'unsigned int (*)(struct azx *, struct azx_dev *)' lvalue
|-ImplicitCastExpr 0x556184b67968 'azx_get_pos_callback_t *' <ArrayToPointerDecay>
| `-MemberExpr 0x556184b67938 'azx_get_pos_callback_t [2]' lvalue ->get_position 0x5561849ca318
|   `-ImplicitCastExpr 0x556184b67920 'struct azx *' <LValueToRValue>
|     `-DeclRefExpr 0x556184b67900 'struct azx *' lvalue ParmVar 0x55618462c178 'chip' 'struct azx *'
`-ImplicitCastExpr 0x556184b678e8 'int' <LValueToRValue>
  `-DeclRefExpr 0x556184b678c8 'int' lvalue Var 0x556184b66bc0 'stream' 'int'
