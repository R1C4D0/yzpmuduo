#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "noncopyable.h"

class EventLoopThreadPool : noncopyable {
   public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    // 在多线程中，baseLoop_获取下一个subloop的方式
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

   private:
    // baseLoop_是主线程的EventLoop对象,也就是Acceptor所在的EventLoop,
    // 也就是mainReactor, 也就是说, 一个EventLoopThreadPool对象,
    // 对应一个mainReactor, 对应一个Acceptor
    EventLoop *baseLoop_;
    std::string name_;
    // 线程池中的线程数也就是subReactor的个数
    int numThreads_;
    bool started_;
    // 线程池中的下一个subReactor的索引
    int next_;
    // 线程池中的线程列表
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    // 线程池中的线程列表中的每一个线程所对应的EventLoop对象的列表
    std::vector<EventLoop *> loops_;
};
