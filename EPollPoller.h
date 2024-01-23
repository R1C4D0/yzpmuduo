#pragma once
#include <sys/epoll.h>

#include <vector>

#include "Poller.h"
#include "Timestamp.h"

/**
 * @brief epoll IO复用的具体实现
 */

class EPollPoller : public Poller {
   public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    // 继承自Poller基类的
    // protected:
    // using ChannelMap = std::unordered_map<int, Channel*>;
    // ChannelMap channels_;

   private:
    /**
     * epoll的使用
     * int epoll_create1(int flags);返回一个epoll的句柄epollfd.
     * flags：1、EPOLL_CLOEXEC:含义同open函数的O_CLOEXEC选项；当执行execve创建新进程时，打开的描述符自动关闭p.s:
     * 当使用完毕时，需要调用close关闭epoll实例句柄.2、0：此时flags参数已经被忽略，只是为了兼容老版本的epoll_create函数
     * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
     * add/mod/del可以注册、修改、删除epoll管理的事件
     * int epoll_wait(int epfd,struct epoll_event *events, int maxevents, int
     * timeout); 等待事件的发生，系统会把所有发生的事件填充到events数组中
     */
    int epollfd_;

    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    //  更新poller监控的Channel
    void update(int operation, Channel* Channel);
    // poller监控的Channel列表
    using EventList = std::vector<epoll_event>;
    EventList events_;
};