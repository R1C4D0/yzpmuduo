/**
 * poller的默认实现类，可以通过Poller的静态方法static Poller*
 * newDefaultPoller(EventLoop* loop)获取默认的IO复用的具体实现
 * 该方法会根据环境变量MUDUO_USE_POLL的值来决定使用poll还是epoll，如果环境变量MUDUO_USE_POLL存在且值为1，则使用poll，否则使用epoll
 * 通过此实现类，使得在Poller.h头文件中不需要包含Epollpoller.h头文件，从而达到解耦的目的
 */
#include "EPollPoller.h"
#include "Poller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr;
    } else {
        return new EPollPoller(loop);
    }
}

#