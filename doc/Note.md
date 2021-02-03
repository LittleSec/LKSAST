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
