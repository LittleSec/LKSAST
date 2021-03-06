import json
import os
from collections import deque
import time

# TU.dump.NORMAL
# PtrInfo.dump.FLAT_STRING

# TODO in this file as an attention

fun2json_fn = "fun2json.all.json"
output_dir = "syscall/moonshine"

if not os.path.exists(output_dir):
    os.makedirs(output_dir, exist_ok=True)

with open(fun2json_fn, mode='r', encoding="utf-8") as fr:
    fun2json_map = json.loads(fr.read())

TUsDumpJson = {}
readJsonFn_set = set()
# it is a map, key is xxx, value is a set() contains all __???_sys_xxx
syscall_collection = {}
syscall_set = {}  # auctally it is a map, key is xxx, value is exactly __???_sys_xxx

# TODO: modify here
def isSyscall(f):
    return f.startswith("__x64_sys_") or f.startswith("__ia32_sys_") or \
        f.startswith("__ia32_compat_sys_") or f.startswith("__se_sys_")


print("[!] Reading TU dump...")
for f, j in fun2json_map.items():
    if j not in readJsonFn_set:
        readJsonFn_set.add(j)
        with open(j, mode='r', encoding="utf-8") as fr:
            TUjson = json.loads(fr.read())
        for k, v in TUjson.items():
            TUsDumpJson[k] = v
print("[+] Done read TU dump")
for f, _ in fun2json_map.items():
    # TODO: modify here
    puref = f.split("_sys_")[-1]
    if isSyscall(f):
        if puref not in syscall_collection:
            syscall_collection[puref] = set()
        syscall_collection[puref].add(f)
for pure_sys, syss in syscall_collection.items():
    # syss is a set, values not in the follow order
    # TODO: modify here
    for prefix_sys in ["__x64_sys_", "__se_sys_", "__ia32_sys_", "__ia32_compat_sys_"]:
        f = prefix_sys + pure_sys
        if f in syss and f in TUsDumpJson and \
                TUsDumpJson[f]["callgraph"] != None and "sys_ni_syscall" not in TUsDumpJson[f]["callgraph"]:
            syscall_set[pure_sys] = f
            break
    else:
        # TODO: modify here
        f = "__x64_sys_" + pure_sys
        syscall_set[pure_sys] = f
        print("[!] syscall: {0} is Empty. Default {1}".format(pure_sys, f))

sys2resource_map = {}


with open(os.path.abspath(os.path.join(output_dir, "syscall.set")), mode='w') as fw:
    fw.write("\n".join(syscall_set.values()))


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
            assert(cgnode[1] == "DirectCall")
            if f not in TUsDumpJson:  # eg. ignore function
                # print("  |- [!] ", f, " NOT in fun2json_map")
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


for f in syscall_set.values():
    print("[!] Handling ", f)
    sysresource_map = mergeResource(f)
    sys2resource_map[f] = sysresource_map
    with open(os.path.abspath(os.path.join(output_dir, f)),
              mode='w', encoding="utf-8") as fw:
        json.dump(sysresource_map, fw, indent=2)


def cal2SyscallsDep(sys1, sys2):
    if sys1 == sys2:
        return 0
    weight = 0  # TODO: moonshine default weight is 0
    resource1 = sys2resource_map[sys1]
    resource2 = sys2resource_map[sys2]
    # rn(resource name), at(access type)
    for rn1, at1 in resource1.items():
        if rn1 in resource2:
            at2 = resource2[rn1]
            # if ('R' in at1 and 'W' in at2) or ('W' in at1 and 'R' in at2):
            if 'W' in at1 and 'R' in at2:
                weight += 1
    return weight


print("start cal2SyscallsDep...")
starttime = time.time()
sysdep_w = {}
sysdep_nw = {}
cnt4log = 0
for pure_sys1, s1 in syscall_set.items():
    sysdep_w[pure_sys1] = {}
    sysdep_nw[pure_sys1] = []
    print("[!][{0}] Calculate: {1} - xxx".format(cnt4log //
                                                 len(syscall_set), pure_sys1))
    for pure_sys2, s2 in syscall_set.items():
        # print("[!][{0}] Calculate: {1} - {2}".format(cnt4log, pure_sys1, pure_sys2))
        cnt4log += 1
        sd = cal2SyscallsDep(s1, s2)
        sysdep_w[pure_sys1][pure_sys2] = sd
        if sd != 0:
            sysdep_nw[pure_sys1].append(pure_sys2)

    # break
with open(os.path.abspath(os.path.join(output_dir, "weight")),
          mode='w', encoding="utf-8") as fw:
    json.dump(sysdep_w, fw, indent=2)
with open(os.path.abspath(os.path.join(output_dir, "ms_weight")),
          mode='w', encoding="utf-8") as fw:
    json.dump(sysdep_nw, fw, indent=2)
print("cal2SyscallsDep time: ", time.time()-starttime)
