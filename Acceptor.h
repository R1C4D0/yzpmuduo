#pragma once

#include <functional>

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "noncopyable.h"

class Acceptor : noncopyable {
   public:
    using NewConnectionCallback =
        std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();
    void listen();
    bool listenning() const { return listenning_; }
    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        newConnectionCallback_ = cb;
    }

   private:
    void handleRead();
    // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};