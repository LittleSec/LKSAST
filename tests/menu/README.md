# 借用作为测试 demo 之一
1. 版权归属：https://github.com/mengning/menu.git 
2. 所作修改如下：
    + fix: 部分使用默认类型强转的代码修复成类型一致，不使用类型转换，以防ub
    + remove: 删去.git不使用原来的git仓库进行管理，而是随主项目一起管理；同时只保留test_exec.c而删除其他test*.c
    + adapt: 修改了Makefile使其能顺便编译出ast文件已经生成含有绝对路径的astList.txt
    + other: 将所有代码合并成了一个源文件，用于ast对比调试
3. 选用该项目作为测试demo之一的原因：
    + 规模大小合适，容易看懂，能人工校对，方便扩展规模和其他情况
    + 含有与Linux Kernel相似的函数指针情况，即struct某个域是函数指针类型，通过初始化或者函数传参进行赋值，但目前还有一点没被覆盖到，即函数指针数组的情况
    + 这些函数指针相关变量赋值后变不再变化
4. 对于原有文件的修改尽量不要对其使用本项目.clangformat进行格式化，而是保留原有的格式


# 以下是原仓库的README

menu
====

cmdline menu libary

example code 
----

    #include <stdio.h>
    #include <stdlib.h>
    #include "menu.h"
    
    int Quit(int argc, char *argv[])
    {
        /* add XXX clean ops */
        exit(0);
    }
    
    int main()
    {
    
        MenuConfig("version","XXX V1.0(Menu program v1.0 inside)",NULL);
        MenuConfig("quit","Quit from XXX",Quit);
        
        ExecuteMenu();
    }


## User Links

* [工程化编程实战](https://mooc.study.163.com/course/1000103000?_trace_c_p_k2_=2b68c5974ba8438894c4518ef342e21b&share=2&shareId=1000001002#/info)
* [代码中的软件工程](https://gitee.com/mengning997/se/blob/master/README.md#%E4%BB%A3%E7%A0%81%E4%B8%AD%E7%9A%84%E8%BD%AF%E4%BB%B6%E5%B7%A5%E7%A8%8B)
* [庖丁解牛Linux内核](https://mooc.study.163.com/course/1000072000?_trace_c_p_k2_=3f48b65c40864fdba6b25b986796ac82&share=2&shareId=1000001002#/info)
