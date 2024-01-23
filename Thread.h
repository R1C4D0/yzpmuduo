#pragma once

#include <string.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "noncopyable.h"
class Thread : noncopyable {
   public:
    using ThreadFunc = std::function<void()>;
    // 传入线程函数和线程名字
    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    bool joined() const { return joined_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }

   private:
    //    用于设置线程的名字以及全局产生的线程的数量
    void setDefaultName();

    //    标识线程的状态
    bool started_;
    bool joined_;
    // 线程对象的生命周期和线程函数的执行周期不一致，线程函数执行完毕后，线程对象还需要继续存在
    // 使用智能指针管理线程对象还可避免内存泄漏
    // 使用智能指针可以控制线程对象的启动和销毁时机，如果直接定义线程对象会直接启动线程
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    //   线程函数
    ThreadFunc func_;
    std::string name_;
    // 用于记录产生的线程的数量
    static std::atomic_int numCreated_;
};
