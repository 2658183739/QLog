#ifndef QSLOGLEVEL_H
#define QSLOGLEVEL_H

namespace QsLogging
{
// 日志级别枚举，用于指定每条日志消息的严重性
enum Level
{
    // Trace级别：最详细的日志，用于跟踪程序执行流程
    TraceLevel = 0,
    // Debug级别：用于调试，提供更详细的程序状态信息
    DebugLevel,
    // Info级别：提供程序运行的通用信息，通常用于记录重要事件
    InfoLevel,
    // Warn级别：表示可能存在问题的情况，但程序可以继续运行
    WarnLevel,
    // Error级别：表示程序中发生了错误，影响了部分功能
    ErrorLevel,
    // Fatal级别：表示发生了致命错误，程序无法继续执行
    FatalLevel,
    // Off级别：一个特殊的级别，用于完全关闭日志记录
    OffLevel
};

}

#endif // QSLOGLEVEL_H
