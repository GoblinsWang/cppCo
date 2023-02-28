# 项目概述

- 可用于高并发网络编程的 C++11 对称协程库
- 使用了成熟的上下文切换方案（glibc 的 ucontext）

## 框架图
![image](https://user-images.githubusercontent.com/45566796/221780974-a552301a-a4ea-4048-9d93-c76a5ee90252.png)

协程框架如上图所示，cppCo 首先会创建一个 Schedular 全局管理的单例，Schedular 单例会根据当前计算的核心数创建对应数据的线程数运行协程，其中每个线程对于一个 Processor 实例。协程库收到的协程任务会被 Schedular 通过特定算法分配到一个 Processor 实例的主循环中，Processor 实例使用 epoll 和定时器 timer 进行任务调度。
