## 代码中的注释

### 1
FieldDecl maybe not a FunctionPointerType, it can be a (void*),
So, do not assert it.
`// (void*) assert(fdit->getType()->isFunctionPointerType());`

### 2
Why not check isPointerType? eg.
regs->ip = (unsigned long) ksig->ka.sa.sa_handler;
```
// if (Ty->isPointerType()) {
// }
```

### 3
Not need to check field for Union variable, because all fields store in same addr in a Union variable 

### About Terminator/TerminatorStmt
1. `eg. if (a < b)`
    + TerminatorStmt: if (a < b)
    + TerminatorCondition: a < b
2. list of `block->getTerminatorStmt()->getStmtClassName()`(maybe more)
```c++
switch (s->getStmtClass()) {
    case Stmt::IfStmtClass:
        /* if, else, else if */
    case Stmt::SwitchStmtClass:
        /* switch case */
    case Stmt::ConditionalOperatorClass:
    case Stmt::BinaryConditionalOperatorClass:
        /* ? : */
    case Stmt::BreakStmtClass:
    case Stmt::ContinueStmtClass:
        /* */
    case Stmt::DoStmtClass:
    case Stmt::WhileStmtClass:
    case Stmt::ForStmtClass:
        /* loop */
    case Stmt::GotoStmtClass:
    case Stmt::IndirectGotoStmtClass:
        /* goto */
    case Stmt::GCCAsmStmtClass:
    case Stmt::MSAsmStmtClass:
        /* asm code */
    case Stmt::BinaryOperatorClass:
        /* &&, ||, ! */
        break;
    default:
        llvm::errs() << s->getStmtClassName() << ": ";
        block->printTerminator(llvm::errs(), LangOptions());
        llvm::errs() << "\n";
        s->dump();
        break;
}
```
3. 不同TerminatorStmt的处理方式：
    + if/switch/goto*中的条件直接影响分支，因此肯定要处理分支
    + for/do/while中的条件主要影响循环次数，当然多少也会影响是否进入循环体，因此也处理分支
    + 剩下的三目运算符和二元操作更多时候是前述stmt中条件的嵌套，或者影响其他变量从而可能影响程序执行路径。因此这类stmt的处理方法值得商榷。可以只处理条件和逻辑表达式语句，也可以所有sub-expr都处理，这里选择后者

### system call declaration
+ include/linux/syscalls.h
+ include/linux/compat.h
+ arch/x86/entry/syscalls/syscall_64.tbl

### Makefile
KBUILD_CFLAGS += -fno-builtin-bcmp

### .config/Kconfig
```
# Coverage collection.
CONFIG_KCOV=y

# Debug info for symbolization.
CONFIG_DEBUG_INFO=y

# Memory bug detector
CONFIG_KASAN=y
CONFIG_KASAN_INLINE=y

# Required for Debian Stretch
CONFIG_CONFIGFS_FS=y
CONFIG_SECURITYFS=y
```

### use clang build kernel
>refer to: https://github.com/ClangBuiltLinux/continuous-integration2/tree/main/tuxsuite

| version | make variable |   syscall  |  note  |
| ------- | ------------- | ---------- | ------ |
|   4.13  | CC=clang      |    SYSC_   |        |
|   4.14  | CC=clang      |    SYSC_   |        |
|   4.15  | CC=clang      |    SYSC_   |        |
|   4.19  | CC=clang      | __x64_sys_ |        |
|   5.3   | CC=clang      | __x64_sys_ | unused |
|   5.8   | LLVM=1        | __x64_sys_ | unused |

1. v4.x 需要在 Makefile 里加入`KBUILD_CFLAGS += -fno-builtin-bcmp`
    + http://lkml.iu.edu/hypermail/linux/kernel/1903.1/04451.html
2. v4.19-v4.19.47在release没有对clang完整的测试因此需要打patch才能使用clang编译
    + https://ask.csdn.net/questions/1487514
    + https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=fd45cd4530ebc7c846f83b26fef526f4c960d1ee
