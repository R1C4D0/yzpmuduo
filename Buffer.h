#pragma once

#include <algorithm>
#include <string>
#include <vector>

// 网络库底层的缓冲器类型定义
class Buffer {
   public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readerIndex_; }

    // onMessage string <- Buffer
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            // 应用只读取了刻度缓冲区数据的一部分，就是len
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }
    void retrieveAll() { readerIndex_ = writerIndex_ = kCheapPrepend; }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len) {
        // 将缓冲区中的数据转成string类型的数据返回，同时对缓冲区进行复位
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    void ensureWriteableBytes(size_t len) {
        // 如果可写的缓冲区长度小于len，就扩容
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }
    void append(const char* data, size_t len) {
        // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
    // 通过fd读写数据
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);

   private:
    char* begin() {
        // vector底层数组的首地址，也就是缓冲区的起始地址
        return &*buffer_.begin();
    }
    const char* begin() const { return &*buffer_.begin(); }
    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                      begin() + kCheapPrepend);
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
