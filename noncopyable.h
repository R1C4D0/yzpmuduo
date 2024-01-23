#pragma once

/**
 * noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象
 * 无法进行拷贝构造和赋值操作
 */
class noncopyable {
   public:
    //    关闭拷贝构造函数，右值拷贝构造函数。关闭赋值运算符重载函数。
    noncopyable(const noncopyable&&) = delete;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

   protected:
    noncopyable() = default;
    ~noncopyable() = default;
};