#include "Channel.h"

#include "EventLoop.h"
#include "Logger.h"
#include "Timestamp.h"

// EventLoop的作用： 1.管理ChannelList  2.管理Poller
// ChannelList的作用： 1.管理Channel  2.管理Poller
// Poller的作用： 1.管理Channel  2.管理EventLoop
// Channel的作用： 1.管理fd
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      state_(Channel::NEW),
      tied_(false) {}
Channel::~Channel() {}

// Channel的tie方法什么时候调用过？一个TcpConnection新连接创建的时候
// TcpConnection => Channel
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}
/**
 * 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * EventLoop => ChannelList   Poller
 */
void Channel::update() {
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}
// 在channel所属的EventLoop中， 把当前的channel删除掉
void Channel::remove() { loop_->removeChannel(this); }
// 得到了poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    // 另一端的socket已经关闭，并且读缓冲区已经读完
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    // 发生错误
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }
    // 有数据可读，或者有紧急数据可读，或者对端关闭连接
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);
    }
    // 可写
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}