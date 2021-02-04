import os
import json
import time

sys2resource_map = {}
syscall_set = set()
sys2resource_dir = "../build/syscall"

for f in os.listdir(sys2resource_dir):
    if f.startswith("__se_sys_"):
        syscall_set.add(f)

for sc in syscall_set:
    with open(os.path.join(sys2resource_dir, sc)) as fr:
        sys2resource_map[sc] = json.loads(fr.read())


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
            if at1 == 'R' and 'W' in at2:
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
with open("weight_ignsame",
          mode='w', encoding="utf-8") as fw:
    json.dump(sysdep, fw, indent=2)
print("cal2SyscallsDep time: ", time.time()-starttime)
