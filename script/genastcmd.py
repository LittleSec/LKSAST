#!/usr/bin python3
import os
import json

# =====###### must run make before runing this script #####=====

# 功能
# 1 提取 compile_commands.json(由bear生成) 里的编译命令
# 2 替换编译器名, 加上 -emit-ast 选项, 修改 -o 参数(以提高文件名的可读性), 其中
#     2.0 检查该编译单元是否需要跳过
#     2.1 默认编译器名在第1个参数, 改成依赖于 shell 环境的 $CC和 $CXX
#     2.2 -emit-ast 放在编译器名后面, 即第2个参数
#     2.3 将 -o 对应参数里的 .o 换成 .ast 后缀, 不修改路径(即不将 ast 文件都放到统一目录下, 而是跟随项目 makefile 里中间文件的生成规则)
# 3 将修改后的编译命令整合到的 buildast.sh 文件
# 4 生成 astList.txt, 存放所有ast文件的绝对路径
# 5 将跳过的编译命令存放到 skip_compile_commands.json
# 其他
# 1 生成的 buildast.sh 支持并行编译生成 ast 文件
# 2 在 in ubuntu 18.04, bear 2.3.11, llvm 9 环境下对 cpython3.8, mysql-5.6.46, **linux-5.8** 测试
# 3 TODO: 应当过滤/跳过非 c/cpp 源码的文件, 目前支持 linux-5.8 需要过滤/跳过汇编文件

expected_json_format = """
// in ubuntu 18.04, bear 2.3.11
[
  {
    "directory": "xxx",
    "arguments": [
      "xxx",
      "xxx",
      ...
    ],
    "file": "xxx"
  },
  {
    ...
  },
  ...
]

OR

// in ubuntu 16.04, bear 2.1.5
[
  {
    "directory": "xxx",
    "command": "xxx",
    "file": "xxx"
  },
  {
    ...
  },
  ...
]
"""

compile_commands_files = "compile_commands.json"

env_clang = "$CC"
env_clangplus = "$CXX"
clang_emit_ast_opt = "-emit-ast"
astfile_ext = ".ast"

# output of this script
buildast_sh_list = []
buildast_sh_fn = "buildast.sh"
astFile_list = []
astFile_fn = "astList.txt"
skipcmd_list = []
skipcmd_fn = "skip_compile_commands.json"

if not os.path.exists(compile_commands_files):
    print("[-] compile_commands.json not exist!")
    exit(1)

def logFormatErr():
    print("[-] compile_commands.json format error!")
    print("[-] expected json format below:")
    print(expected_json_format)

with open(compile_commands_files, mode='r') as fr:
    ccjson = json.loads(fr.read())

if type(ccjson) is not list:
    logFormatErr()
    exit(1)

# prepare bash script
parallel_script_prepare = '''#!/bin/bash

if [ 1 -ne $# ]; then
  Nproc=$(nproc)
else
  Nproc=$1
fi

TMPFILE=./$$.ast.fifo
[ -e $TMPFILE ] || mkfifo $TMPFILE
exec 4<> $TMPFILE
rm -rf $TMPFILE
for i in `seq 1 $Nproc`; do
  echo "genast_parallel_token" >&4
done

echo "[+] Start build ast (-j$Nproc) ......"

'''
############# -emit-ast #############
# read -u 4
# {
#   # my cmd
#   echo "genast_parallel_token" >&4
# } &
############# -emit-ast #############
parallel_read_fifo_token = "read -u 4"
parallel_write_fifo_token = "echo \"genast_parallel_token\" >&4"
parallel_script_finish = '''
wait
exec 4>&-
exec 4<&-
echo "[+] done!"
'''

buildast_sh_list.append(parallel_script_prepare)
buildast_sh_list.append("# Parallel Gen .ast file cmd as follow...")

for cc in ccjson:
    cmdargs = cc.get("arguments") # ubuntu 18.04, bear 2.3.11
    directory = cc.get("directory")
    filename = cc.get("file")

    if cmdargs is None:
        command = cc.get("command") # ubuntu 16.04, bear 2.1.5
        cmdargs = command.split(' ')

    if None in [cmdargs, directory, filename]:
        logFormatErr()
        exit(1)
    
    # filter not c/cpp language file
    # filter assembly code
    ext = os.path.splitext(filename)[-1]
    if ext.lower() == ".s":
        skipcmd_list.append(cc)
        continue
    # Ugly:
    # if ext.lower() not in [".c", ".cpp", ".cc", ".cxx", ".c++", ".h", ".hpp", ".hxx", ".h++"]:
    #     continue

    if "-o" in cmdargs:
        outopt_idx = cmdargs.index("-o")
        output_old = cmdargs[outopt_idx+1]
        # This script assumes that the `make` program has been run before running,
        # so the file path must exist, no need to determine whether the path exists
        output_new = os.path.splitext(output_old)[0] + astfile_ext
        if not os.path.isabs(output_old):
            output_new = os.path.join(directory, output_new)
        astFile_list.append(output_new)
        cmdargs[outopt_idx+1] = output_new
        # change C compile into clang and add option `-emit-ast`
        if cmdargs[0][-3:].lower() in ["c++", "g++", "cpp", "cxx"]:
            cmdargs[0] = ' '.join([env_clangplus, clang_emit_ast_opt])
        else:
            cmdargs[0] = ' '.join([env_clang, clang_emit_ast_opt])
        # cmdargs[-1] = os.path.join(directory, filename)
        # in ubuntu 16.04, bear 2.1.5, Linux Kernel(LLVM=1):
        # e.g. "-DBUILD_STR(s)=#s" => -DBUILD_STR(s)=#s => ..
        for i, arg in enumerate(cmdargs):
            if arg.startswith('"') and arg.endswith('"'):
              cmd_exe[i] = arg[1:-1]
        cmd_exe = ' '.join(cmdargs)
        cmd_exe = cmd_exe.replace('"', r'\"') # e.g. -DMARCO="string", the double quotation marks should be escaped => -DMARCO=\"string\"
        # in Linux Kernel(LLVM=1): 
        # -DBUILD_STR(s)=#s => -DBUILD_STR\(s\)=\#s
        #                or => -D"BUILD_STR(s)=#s"
        cmd_exe = cmd_exe.replace('#', r'\#')
        cmd_exe = cmd_exe.replace('(', r'\(')
        cmd_exe = cmd_exe.replace(')', r'\)')
        cmd_exe = cmd_exe.replace(r'\\"', r'\"') # in ubuntu 16.04, bear 2.1.5 will handle the double quotation, so here reverse last instructions
        # cd directory
        cmd_cd = ' '.join(["cd", directory])
        abs_filepath = os.path.join(directory, filename)
        abs_filepath = os.path.abspath(abs_filepath)
        # print(cmd_exe)
        ############# -emit-ast #############
        # read -u 4
        # {
        #   my_cmd && (
        #     echo "[+] succ: xxx.c"
        #   ) || (
        #     echo "[-] fail: xxx.c"
        #     kill -9 0
        #   )
        #   echo "genast_parallel_token" >&4
        # } &
        ############# -emit-ast #############
        buildast_sh_list.append(parallel_read_fifo_token)
        buildast_sh_list.append("{")
        buildast_sh_list.append("  " + cmd_cd)
        buildast_sh_list.append("  " + cmd_exe + " && (")
        buildast_sh_list.append("    echo \"[+] succ: " + abs_filepath + "\"")
        buildast_sh_list.append("  ) || (")
        buildast_sh_list.append("    echo \"[-] failed: " + abs_filepath + "\"")
        buildast_sh_list.append("    kill -9 0")
        buildast_sh_list.append("  )")
        buildast_sh_list.append("  " + parallel_write_fifo_token)
        buildast_sh_list.append("} &\n")
    else:
        print("[-] Todo(Ops, not support!)")
        exit(1)
buildast_sh_list.append(parallel_script_finish)

with open(buildast_sh_fn, mode='w', encoding="utf-8") as fw:
    fw.write('\n'.join(buildast_sh_list))

with open(astFile_fn, mode='w', encoding="utf-8") as fw:
    fw.writelines('\n'.join(astFile_list))

with open(skipcmd_fn, mode='w', encoding="utf-8") as fw:
    json.dump(skipcmd_list, fw, indent=4)
