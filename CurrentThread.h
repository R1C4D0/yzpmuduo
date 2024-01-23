#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
extern __thread int t_cachedTid;

void cacheTid();
/**
 * @brief 返回当前线程的tid
 */
inline int tid() {
    // __builtin_expect是GCC内置的函数，是为了优化分支预测，__builtin_expect(表达式，希望的结果)
    // 返回值为表达式的值，但是编译器会认为表达式的值为希望的结果，从而进行优化
    // 这里意思为：编译器优化不执行if语句，除非t_cachedTid==0
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

}  // namespace CurrentThread