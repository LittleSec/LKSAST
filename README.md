# LKSAST
Linux Kernel Static Analysis with clang AST

## environment
1. Ubuntu 18.04
2. LLVM 10.0.0, use Pre-Built Binaries in Ubuntu 18.04
    + TODO: if llvm/clang build from source code, it can build this project, but after build kernel in ast, and this project read from these ast files will throw some errors.


## build
```shell
mkdir build
cd build
cmake -DLLVM_PREFIX=/path/to/llvm10 -DCMAKE_BUILD_TYPE=Debug ..
make -j`nproc`
```

## run
```shell
cd build
./a.out ../config/xxx.json
# The following command will be more common
time ./a.out ../config/xxx.json 2>&1 | tee linux-x.y.log
# it will output some files which specify in config.json
```


## some temporary backup

### kernel build
1. `sudo apt install libelf-dev libssl-dev`

### kernel fuzzing
1. golang 1.14 for syzkaller
2. qemu: `sudo apt install qemu qemu-kvm` is ok
