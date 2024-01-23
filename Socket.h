#pragma once
#include "InetAddress.h"
#include "noncopyable.h"

// 封装socket fd来进行RAII, 用于管理socket的生命周期, 以及对socket的操作,
// 比如bind, listen, accept, shutdownWrite等, 但是不包括close,
// close由TcpConnection来完成, 因为TcpConnection需要在析构时关闭socket,
// 而Socket类的生命周期可能会比TcpConnection的生命周期长,
// 所以不能在Socket的析构函数中关闭socket
class Socket : noncopyable {
   public:
    explicit Socket(int socketfd) : socketfd_(socketfd) {}

    ~Socket();
    int fd() const { return socketfd_; }
    /**
     * @brief 用于为socketfd_绑定服务器端用于通信的地址和端口号
     * @param localaddr 用于保存本地的地址信息
     *
     */
    void bindAddress(const InetAddress &localaddr);
    void listen();
    /**
     * @param peeraddr 用于保存对端的地址信息
     * @param connfd 用于保存accept返回的connfd
     */
    int accept(InetAddress *peeraddr);
    // 关闭写端
    void shutdownWrite();
    // 设置socket选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

   private:
    // Socket类封装的socket fd
    const int socketfd_;
};
