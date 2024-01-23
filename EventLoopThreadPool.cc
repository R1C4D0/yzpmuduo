#include "EventLoopThreadPool.h"

#include <memory>

#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,
                                         const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      numThreads_(0),
      started_(false),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;
    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        // 底层创建线程，cb是线程初始化回调函数
        EventLoopThread *t = new EventLoopThread(cb, buf);
        // threads_是线程池中的线程列表
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // loops_是线程列表中的每一个线程所对应的EventLoop对象的列表
        loops_.push_back(t->startLoop());
    }
    // 此时，整个服务端只有一个线程，运行着baseloop
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}
// 在多线程中，baseLoop_获取下一个subloop用以分配Channel的方式，这里使用轮询
EventLoop *EventLoopThreadPool::getNextLoop() {
    // 当只有一个线程时，baseLoop_就是唯一的subloop
    EventLoop *loop = baseLoop_;
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop *>(1, baseLoop_);
    } else {
        return loops_;
    }
}