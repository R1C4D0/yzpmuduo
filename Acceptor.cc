#include "Acceptor.h"

#include <errno.h>

#include "InetAddress.h"
#include "Logger.h"

static int createNonblocking() {
    int sockfd =
        ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__,
                  __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr,
                   bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    // 绑定监听地址
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start() Acceptor.listen
    // 有新用户的连接，要执行一个回调（connfd=》channel=》subloop） baseLoop =>
    // acceptChannel_(listenfd) =>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}
void Acceptor::listen() {
    listenning_ = true;
    // 底层调用listen函数进行fd监听
    acceptSocket_.listen();
    // 将acceptChannel_的读事件注册到poller中，让poller监听，有新连接到来时，会通知acceptChannel_
    acceptChannel_.enableReading();
}
// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead() {
    InetAddress peerAddr;
    // 调用Socket的accept函数，获取新连接的fd,对端地址
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd > 0) {
        // 如果有新连接到来，调用newConnectionCallback_回调函数
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            // 如果没有设置回调函数，直接关闭新连接
            ::close(connfd);
        }
    } else {  // accept出错
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__,
                  errno);
        // 表示连接数达到上限，不再接受新连接
        if (errno == EMFILE) {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__,
                      __FUNCTION__, __LINE__);
        }
    }
}
