#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Channel.h"
#include "CurrentThread.h"
#include "Poller.h"
#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop : noncopyable {
   public:
    using Fuctor = std::function<void()>;  // 回调操作类型

    EventLoop();
    ~EventLoop();

    // 开启/退出事件循环
    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Fuctor cb);
    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Fuctor cb);

    // 用来唤醒loop所在的线程的
    void wakeup();

    // EventLoop的方法调用Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

   private:
    void handleRead();         // wake up
    void doPendingFunctors();  // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;  // 原子操作，通过CAS实现的
    std::atomic_bool quit_;     // 标识退出loop循环

    const pid_t threadId_;  // 记录当前loop所在线程的id

    Timestamp pollReturnTime_;  // poller返回发生事件channels的时间点
    std::unique_ptr<Poller>
        poller_;  // 每个loop都有一个poller对象，用来执行IO复用

    int wakeupFd_;  // 用来唤醒loop所在的线程的,
                    // 通过eventfd来实现，各个线程之间的通信机制
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;  // 记录发生事件的channels

    // 标志当前loop是否需要执行回调操作
    std::atomic_bool callingPendingFunctors_;
    std::vector<Fuctor> pendingFunctors_;  // 回调操作队列
    std::mutex mutex_;  // 互斥锁，用来保护上面vector容器的线程安全操作
};
