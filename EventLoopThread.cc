#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : exiting_(false),
      loop_(nullptr),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb) {}
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        // 等待管理的线程结束
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    thread_.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待EventLoop对象创建完毕
        // 相当于cond_.wait(lock, [this]{return loop_ != nullptr;});
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
        return loop;
    }
}
// 新线程的入口函数
void EventLoopThread::threadFunc() {
    // 创建一个独立的eventloop，和线程是一一对应的，
    // one loop per thread，之后会传递给EvnentLoopThread的loop_成员
    // loop是栈上的对象，线程退出时会自动销毁
    EventLoop loop;
    if (callback_) {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 传递EventLoop对象给EventLoopThread，并通知startLoop中的wait
        loop_ = &loop;
        cond_.notify_one();
    }
    // EventLoop loop  => Poller.poll
    loop.loop();
    // 到这里时，loop.loop()已经退出，说明EventLoopThread管理的线程也要退出了
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}