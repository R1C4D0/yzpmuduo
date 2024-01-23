#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>

#include "EventLoop.h"
#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop;
/**
 * 理清楚  EventLoop、Channel、Poller之间的关系   《= Reactor模型上对应
 * Demultiplex Channel
 * 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */
class Channel : noncopyable {
   public:
    // channel在对应poller中的状态,未添加，已添加，已删除,因为后面操作使用与或非，所以不用枚举类型Enum
    static const int NEW = -1;
    static const int ADDED = 1;
    static const int DELETED = 2;
    // channnel监听的fd的状态 EPOLLIN: 有数据可读, EPOLLPRI:
    // 有紧急数据可读。EPOLLOUT: 可写
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();
    // 处理poller通知的channel发生的具体事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) {
        revents_ = revt;
        return revents_;
    }
    // 设置fd相应的事件状态
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll() {
        events_ = kNoneEvent;
        update();
    }
    // 返回fd的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // for Poller
    int state() { return state_; }
    void set_state(int state) { state_ = state; }

    // one loop per thread, 一个eventloop只能属于一个线程
    EventLoop *ownerLoop() { return loop_; }
    // 从EventLoop中删除当前的channel
    void remove();

   private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // Channel所属的EventLoop
    EventLoop *loop_;
    // Channel所管理的fd
    const int fd_;
    // Channel所感兴趣的事件
    int events_;
    // poller返回的发生的具体事件
    int revents_;
    // 标志channel在poller中的状态
    int state_;
    // 用于绑定TcpConnection和Channel，防止TcpConnection被手动remove掉，Channel还在执行回调操作
    std::weak_ptr<void> tie_;
    bool tied_;

    // 回调函数对象,
    // 由TcpConnection传入,channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};