#include "EventLoop.h"

#include <sys/eventfd.h>

#include "Logger.h"

// one loop per thread,
// thread_local关键字的作用是保证每个线程都只能有一个独立的EventLoop对象
thread_local EventLoop* t_loopInThisThread = nullptr;
// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_{CurrentThread::tid()},
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",
                  t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的回调操作，每一个eventloop都会被wakeupchannel的EPOLLIN读事件唤醒
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()

{
    // 将wakechannel从poller中移除,释放wakeupfd
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    // 释放当前线程的eventloop对象
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_) {
        activeChannels_.clear();
        // 调用poller的poll函数，获取当前活跃的channel，一类是wakeupchannel，一类是subreactor监听的channel
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // 调用每一个channel的handleEvent
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前eventloop事件循环需要处理的回调操作
        /**
         * mainLoop事先注册了一个回调cb（需要执行的函数）给subLoop，subLoop在处理完事件后，会调用这个回调cb
         * wakeup
         * subloop后，subloop会执行这个回调cb，这个回调cb就是执行mainloop注册的的回调操作
         */
        doPendingFunctors();
    }
    // 事件循环结束
    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}
/**
 * 1.如果当前线程是创建eventloop的线程，直接退出
 * 2.如果当前线程不是创建eventloop的线程，需要唤醒创建eventloop的线程，让其退出
 */
/*
            mainloop

    subloop1 subloop2  subloop3
*/

void EventLoop::quit() {
    quit_ = true;
    // 如果当前线程不是创建eventloop的线程，需要唤醒创建eventloop的线程，让其退出
    if (!isInLoopThread()) {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Fuctor cb) {
    // 如果当前线程是创建eventloop的线程，直接执行cb
    if (isInLoopThread()) {
        cb();
    }
    // 如果当前线程不是创建eventloop的线程，需要唤醒创建eventloop的线程，让其执行cb
    else {
        queueInLoop(std::move(cb));
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Fuctor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    // 唤醒相应的，需要执行上面回调操作的,loop所在的线程
    // callingPendingFunctors_意思是：当前loop正在执行回调，但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

// 用来唤醒loop所在的线程的,通过eventfd来实现各个线程之间的通信机制,向wakeupfd写一个八字节数据即可
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法调用Poller的方法
void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}
// 这是wakeupChannel的读回调操作，主要作用处理本线程被唤醒后，需要执行的回调操作
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}
void EventLoop::doPendingFunctors() {
    std::vector<Fuctor> fuctors;
    callingPendingFunctors_ = true;
    // lock_guard是一个函数对象，构造时会自动调用lock()，析构时会自动调用unlock(）
    // 直接交换两个vector，将原来的取出并清空，交换后的vector在临界区外执行,减少临界区的长度
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fuctors.swap(pendingFunctors_);
    }
    for (const Fuctor& fuctor : fuctors) {
        fuctor();
    }
    callingPendingFunctors_ = false;
}