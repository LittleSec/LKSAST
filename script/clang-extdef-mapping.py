#!/usr/bin python3
import os
import json

# compile_commands.json -> extract_extdef_map.sh
# --> exec cmd: clang-extdef-mapping -p . xxx.file.c
# --> cmd output: c:@F@xxxfunc xxx.file.c

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

env_clangtool = "$CLANGTOOL" # "clang-extdef-mapping"
clangtool_opt = "-p" # -p=<string>     - Build path
compile_db_path = "."
extdef_mapping_txt = "extdef_mapping.txt"

# output of this script
extractextdefmap_sh_list = []
extractextdefmap_sh_fn = "extract_extdef_map.sh"

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

script_prepare = '''#!/bin/bash
set -e

: > {0}
'''.format(extdef_mapping_txt)
script_finish = '''
echo "[+] done!"
'''

extractextdefmap_sh_list.append(script_prepare)

procNum = len(ccjson)
for i, cc in enumerate(ccjson):
    cmdargs = cc.get("arguments") # ubuntu 18.04, bear 2.3.11
    directory = cc.get("directory")
    filename = cc.get("file")

    if cmdargs is None:
        command = cc.get("command") # ubuntu 16.04, bear 2.1.5
        cmdargs = command.split(' ')

    if None in [cmdargs, directory, filename]:
        logFormatErr()
        exit(1)
    
    absfilepath = os.path.join(directory, filename) # FIXME?

    # filter not c/cpp language file
    # filter assembly code
    ext = os.path.splitext(filename)[-1]
    if ext.lower() == ".s":
        echo_exe = "echo \"[ ][{0}/{1}] skip: {2}\"\n".format(i+1, procNum, absfilepath)
        extractextdefmap_sh_list.append(echo_exe)
        continue
    # Ugly:
    # if ext.lower() not in [".c", ".cpp", ".cc", ".cxx", ".c++", ".h", ".hpp", ".hxx", ".h++"]:
    #     continue

    echo_exe = "echo \"[!][{0}/{1}] processing: {2}\"".format(i+1, procNum, absfilepath)
    cmd_exe = ' '.join([env_clangtool, clangtool_opt, compile_db_path, absfilepath])
    cmd_exe += " >> {0}\n".format(extdef_mapping_txt) # clang-extdef-mapping -p . xxx.file.c >> mapping
    extractextdefmap_sh_list.append(echo_exe)
    extractextdefmap_sh_list.append(cmd_exe)
extractextdefmap_sh_list.append(script_finish)

with open(extractextdefmap_sh_fn, mode='w', encoding="utf-8") as fw:
    fw.write('\n'.join(extractextdefmap_sh_list))
