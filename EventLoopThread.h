#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "EventLoop.h"
#include "Thread.h"
#include "noncopyable.h"

class EventLoopThread : noncopyable {
   public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

   private:
    void threadFunc();

    bool exiting_;
    // 一个EventLoopThread对象对应一个线程和一个EventLoop对象
    EventLoop *loop_;
    Thread thread_;
    // 用于线程同步
    std::mutex mutex_;
    std::condition_variable cond_;
    // 线程初始化回调函数
    ThreadInitCallback callback_;
};
