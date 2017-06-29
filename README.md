# libpc

A simple coroutines library -- libpc

------------------------------------------

Author: Pengcheng Liu  
Lpc-Win32 Homepage: www.lpc-win32.com  
Lpc-Win32 Blog    : blog.lpc-win32.com

------------------------------------------

> 仅支持32/64位处理器架构，linux操作系统。g++版本需支持C++11

PC func/lambda/\<functional\> can create a pcoroutine to run function what we want!~~

### libpc versions

当./install.sh完成libpc的安装后，在Linux shell下执行libpc\_version命令即可显示当前版本号

- v1.0-beta-1: libpc created

- v1.0-beta-2: Add a syntactic sugar PC, 当我们启动一个协程执行某些操作时，无需繁琐的创建调度步骤。仅仅 PC(function/lambda/成员方法等均支持)  
BUG: 协程使用过程中存在内存泄露问题

- v1.0-beta-3: 维护协程池，避免反复的malloc与free这样不必要的额外开销，v1.0-beta-2内存泄露BUG  
增加HOOK函数--sleep, usleep。  
注意：HOOK函数usleep的精度只精确到毫秒，暂时不打算对sleep这类函数做过多的兼容与支持。如果一个项目中出现很多sleep，通常需要考虑程序设计的问题。  
sleep时，当前协程让出cpu使用权。待timeout，重新执行

- v1.0-beta-4: 增加pc调度队列，让我们的程序的运行更加随机，脱离线性。程序的编写需要遵守example\_pc\_schedule.cpp方式。  
schedule不会在libpc中进一步封装，封装的工作交由后期基于libpc实现的网络库完成

### TODO

> 已解决问题：问题1、2  
待解决问题：问题3，问题4，问题5，问题6，问题7，问题8

- 问题1：每次创建一个协程，会malloc一个协程栈（128kb空间），使用完成后free掉，反复malloc与free不是一个高效的行为。  
解决方式：维护协程池避免反复的malloc与free。pc\_pool在每个进程中仅有一次的init，在init时需要确定好分配的协程池的容量。该值在程序运行的过程中是不能修改容量的，因为不推荐这样的做法。协程池满的后，则将新malloc协程，执行完毕后free协程空间。因此协程栈的大小需要针对不同的业务而定。目前暂定的协程栈大小为

- 问题2：每次create一个协程，协程并不会自动执行，而是必须我们显示的resume才会执行。往往在现实的应用中，我们更倾向于使用类似与GO的协程行为  
解决方式：维护一个工作队列，协程create时或PC时，将协程任务加入到工作队列中由调度算法进行调度

- 问题3：没有日志管理，不便于问题的排查  
解决方式：写一个。。。

- 问题4：libpc代码健壮性优化

- 问题5：libpc性能优化、优化schedule逻辑，多纬度调度。

- 问题6：增加配置文件读取，提升libpc适应系统环境能力

- 问题7：尽可能多的增加HOOK适配

- 问题8：实现libpc协程锁，避免共用同一资源时yield产生的数据同步问题
