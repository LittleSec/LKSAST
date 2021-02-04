import json
import os
from collections import deque
import time

# TU.dump.NORMAL
# PtrInfo.dump.FLAT_STRING

fun2json_fn = "fun2json.all.json"
ptrinfo_fn = "PtrInfo.all.json"

with open(fun2json_fn, mode='r', encoding="utf-8") as fr:
    fun2json_map = json.loads(fr.read())

with open(ptrinfo_fn, mode='r', encoding="utf-8") as fr:
    ptrinfo_map = json.loads(fr.read())


def isSyscall(f):
    # return f.startswith("__x64_sys_") or f.startswith("__ia32_sys_") or f.startswith("__ia32_compat_sys_") or f.startswith("__se_sys_")
    return f.startswith("__se_sys_")


TUsDumpJson = {}
readJsonFn_set = set()
syscall_set = set()

print("[!] Reading TU dump...")
for f, j in fun2json_map.items():
    if j not in readJsonFn_set:
        readJsonFn_set.add(j)
        with open(j, mode='r', encoding="utf-8") as fr:
            TUjson = json.loads(fr.read())
        for k, v in TUjson.items():
            TUsDumpJson[k] = v
    if isSyscall(f):
        syscall_set.add(f)
print("[+] Done read TU dump")

sys2resource_map = {}


def mergeResource(syscall):
    # init resource and callgraph
    resource_map = {}
    cg_queue = deque()  # type(elem) is tuple
    cg_queue.append((syscall, "DirectCall"))
    hasanalysis_cg_set = {"NULLFunPtr"}  # syscall not in here now
    # start merge
    while (len(cg_queue) != 0):
        cgnode = cg_queue.popleft()
        f = cgnode[0]
        if f in hasanalysis_cg_set:
            continue
        else:
            hasanalysis_cg_set.add(f)
            if (cgnode[1] != "DirectCall"):
                if f in ptrinfo_map:
                    for ptee in ptrinfo_map[f]:
                        cg_queue.append((ptee, "DirectCall"))
            else:
                if f not in TUsDumpJson:  # eg. ignore function
                    print("  |- [!] ", f, " NOT in fun2json_map")
                    continue
                if TUsDumpJson[f]["resource_access"] != None:
                    for k, v in TUsDumpJson[f]["resource_access"].items():
                        # if k.startswith("struct trace_event_raw"):
                        #     continue
                        if k in resource_map:
                            srcRW = resource_map[k]
                            tgrRW = v
                            if (srcRW == 'W' and tgrRW == 'R') or \
                                    (srcRW == 'R' and tgrRW == 'W'):
                                resource_map[k] = "R/W"
                        else:
                            resource_map[k] = v
                if TUsDumpJson[f]["callgraph"] != None:
                    for cgnode in TUsDumpJson[f]["callgraph"].items():
                        cg_queue.append(cgnode)
    return resource_map


if not os.path.exists("syscall"):
    os.mkdir("syscall")

for f in syscall_set:
    print("[!] Handling ", f)
    sysresource_map = mergeResource(f)
    sys2resource_map[f] = sysresource_map
    with open(os.path.abspath(os.path.join("syscall", f)),
              mode='w', encoding="utf-8") as fw:
        json.dump(sysresource_map, fw, indent=2)


def cal2SyscallsDep(sys1, sys2):
    if sys1 == sys2:
        return 0
    weight = 1  # default weight is 1
    resource1 = sys2resource_map[sys1]
    resource2 = sys2resource_map[sys2]
    # rn(resource name), at(access type)
    for rn1, at1 in resource1.items():
        if rn1 in resource2:
            at2 = resource2[rn1]
            if at1 == at2:
                weight += 5
            elif at1 == 'R' and 'W' in at2:
                weight += 1
            elif 'W' in at1 and at2 == 'R':
                weight += 11
    return weight

print("start cal2SyscallsDep...")
starttime = time.time()
sysdep = {}
for s1 in syscall_set:
    sysdep[s1] = {}
    for s2 in syscall_set:
        sysdep[s1][s2] = cal2SyscallsDep(s1, s2)
    # break
with open(os.path.abspath(os.path.join("syscall", "weight")),
          mode='w', encoding="utf-8") as fw:
    json.dump(sysdep, fw, indent=2)
print("cal2SyscallsDep time: ", time.time()-starttime)