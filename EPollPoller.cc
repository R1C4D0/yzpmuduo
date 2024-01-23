#include "EPollPoller.h"

#include <string.h>
#include <unistd.h>

#include "Channel.h"
#include "Logger.h"

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    // epoll_create1(EPOLL_CLOEXEC)创建一个epoll实例失败，直接退出
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() {
    // epoll实例创建成功，关闭epoll实例，epoll实例为系统资源
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__,
             channels_.size());
    // epoll_wait():等待事件的发生，系统内核会把所有发生的事件填充到events数组中,返回发生的事件数，返回0表示超时(经过timeoutMs)
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        LOG_INFO("%d events happened \n", numEvents);
        // 填写活跃的连接
        fillActiveChannels(numEvents, activeChannels);
        // 如果发生的事件数等于events_数组的大小，说明events_数组不够用，需要扩容
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        // 如果发生错误，且错误不是被信号中断，则打印错误日志
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}
// 调用过程：channel update remove => EventLoop updateChannel removeChannel =>
// Poller updateChannel removeChannel
/**
 *       组件结构：EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 */

void EPollPoller::updateChannel(Channel* channel) {
    const int state = channel->state();
    LOG_INFO("func=%s => fd=%d events=%d state=%d \n", __FUNCTION__,
             channel->fd(), channel->events(), state);
    if (state == Channel::NEW || state == Channel::DELETED) {
        if (state == Channel::NEW) {
            // 如果是一个新的channel，需要把这个channel添加到channels_中
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_state(Channel::ADDED);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // channel已经poller上注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            // 如果channel没有事件，需要把这个channel从channels_中删除
            update(EPOLL_CTL_DEL, channel);
            channel->set_state(Channel::DELETED);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);
    int state = channel->state();
    if (state == Channel::ADDED) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_state(Channel::NEW);
}
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const {
    // events_是在fillActiveChannels调用之前就已经通过epoll_wait()系统调用填充好的
    for (int i = 0; i < numEvents; ++i) {
        // events_[i].data.fd是发生事件的文件描述符
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        // 根据文件描述符在map中找到对应的channel
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}
// 更新poller监控的Channel
void EPollPoller::update(int operation, Channel* Channel) {
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = Channel->fd();

    event.events = Channel->events();
    event.data.fd = fd;
    event.data.ptr = Channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl op=%d fd=%d \n", operation, fd);
        } else {
            LOG_FATAL("epoll_ctl op=%d fd=%d \n", operation, fd);
        }
    }
}