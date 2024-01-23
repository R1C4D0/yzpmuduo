网络库使用Multi-Reactor架构，实现了⾮阻塞同步 IO（主从 reactor + 线程池） + IO 多路复⽤（epoll边沿触发）并发模型

实现one loop per thread思想，每个线程管理一个EventLoop，利用线程池实现多线程机制

SubReactor线程池管理默认使用轮询算法，利用eventfd的异步wakeup操作实现各个线程之间的高效通信

IO处理使⽤了epoll_wait + non-blocking 同步IO，使用缓冲区技术管理客户端IO输入和输出

实现异步日志系统，记录服务器运行状态，日志系统使用单例模式获取实例实现日志的写入和读取。

实现了RAII思想：对linux提供的thread，socketfd，eventfd，listenfd，sockaddr_in进行封装，使用智能指针进行管理
