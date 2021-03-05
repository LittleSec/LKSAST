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

| version | make variable |   syscall  |
| ------- | ------------- | ---------- |
|   4.13  | CC=clang      |    SYSC_   |
|   4.15  | CC=clang      |    SYSC_   |
|   4.19  | CC=clang      | __x64_sys_ |
|   5.3   | CC=clang      | __x64_sys_ |
|   5.8   | LLVM=1        | __x64_sys_ |

1. 4.13/4.15 需要在 Makefile 里加入`KBUILD_CFLAGS += -fno-builtin-bcmp`
    + http://lkml.iu.edu/hypermail/linux/kernel/1903.1/04451.html
2. 4.19在release没有对clang完整的测试因此需要打patch才能使用clang编译
    + https://ask.csdn.net/questions/1487514
    + https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=fd45cd4530ebc7c846f83b26fef526f4c960d1ee
