## 代码中的注释

### 1
FieldDecl maybe not a FunctionPointerType, it can be a (void*),
So, do not assert it.
`// (void*) assert(fdit->getType()->isFunctionPointerType());`

### 2
Maybe come from ParmVar, maybe a normal(global/local) variable(with/without init)
DeclRefExpr -> ValueDecl -> VarDecl ->:
```cpp
if (VD->isFunctionOrFunctionTemplate()) {
} else if (VD->getType()->isFunctionPointerType()) {
} else {
  // here
}
```

### 3
DeclRefExpr -> ValueDecl -> :
```cpp
if (VarDecl *VD = dyn_cast<VarDecl>(ValueD)) {
} else if (FunctionDecl *FD = dyn_cast<FunctionDecl>(ValueD)) {
} else {
  // here
}
```
