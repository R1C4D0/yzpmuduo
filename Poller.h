#pragma once

#include <unordered_map>
#include <vector>

#include "Timestamp.h"
#include "noncopyable.h"

class Channel;
class EventLoop;

class Poller : noncopyable {
   public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop);
    virtual ~Poller() = default;
    // 给所有IO复用保留统一的接口，这是纯虚函数，具体的实现由子类（EpollPoller等）完成
    /**
     * @param timeoutMs 超时时间
     * @param activeChannels 保存发生事件的Channel
     */
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* Channel) = 0;

    bool hasChannel(Channel* channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

   protected:
    /**
     map的key：sockfd  value：sockfd所属的channel通道类型
     *
    */
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

   private:
    // 定义Poller所属的事件循环EventLoop
    EventLoop* ownerLoop_;
};