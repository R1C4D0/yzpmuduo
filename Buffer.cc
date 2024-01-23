#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据  Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    // 栈上的内存空间  64K
    char extrabuf[65536] = {0};
    struct iovec vec[2];
    // 这是Buffer底层缓冲区剩余的可写空间大小
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // 一次最多允许读取数据大小为65536+缓冲区可写空间大小
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writable)  // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    } else  // extrabuf里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        // writerIndex_开始写 n - writable大小的数据
        append(extrabuf, n - writable);
    }

    return n;
}
ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}