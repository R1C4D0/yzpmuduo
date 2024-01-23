#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Acceptor.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "noncopyable.h"

// 对外的服务器编程使用的类

class TcpServer : noncopyable {
   public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr,
              const std::string &nameArg, Option option = kNoReusePort);

    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback &cb) {
        threadInitCallback_ = cb;
    }
    void setConnectionCallback(const ConnectionCallback &cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

   private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    // baseLoop 用户定义的loop
    EventLoop *loop_;
    //  服务器监听的地址
    const std::string ipPort_;
    const std::string name_;
    // 运行在baseLoop上的acceptor,任务是监听新连接的到来
    std::unique_ptr<Acceptor> acceptor_;
    // 线程池，实现了一个简单的负载均衡，one loop per thread
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;  // 有新连接时的回调
    MessageCallback messageCallback_;        // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;  // 消息发送完成以后的回调

    ThreadInitCallback threadInitCallback_;  // loop线程初始化的回调

    std::atomic_int started_;  // 服务器是否已经启动
                               // 连接总数计数
    int nextConnId_;
    // TcpConnection的管理
    ConnectionMap connections_;
};