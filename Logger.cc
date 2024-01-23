#include "Logger.h"

#include <iostream>

#include "Timestamp.h"

// 获取日志类的单例对象,单例模式，使用函数内的静态局部变量
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}
// 设置日志级别
void Logger::setLogLevel(LogLevel level) { logLevel_ = level; }
// 写日志
void Logger::log(std::string msg) {
    switch (logLevel_) {
        case INFO:
            std::cout << "[INFO]";
            break;
        case ERROR:
            std::cout << "[ERROR]";
            break;
        case FATAL:
            std::cout << "[FATAL]";
            break;
        case DEBUG:
            std::cout << "[DEBUG]";
            break;
        default:
            break;
    }
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}