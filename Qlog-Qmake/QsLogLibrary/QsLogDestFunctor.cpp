#include "QsLogDestFunctor.h"
#include <cstddef>
#include <QtGlobal>

// 使用函数作为日志回调
QsLogging::FunctorDestination::FunctorDestination(LogFunction f)
    : QObject(NULL) // 继承 QObject，但父对象为空
    , mLogFunction(f) // 将传入的函数指针赋给成员变量
{
}

// 使用 Qt 信号-槽作为日志回调
QsLogging::FunctorDestination::FunctorDestination(QObject *receiver, const char *member)
    : QObject(NULL) // 继承 QObject，但父对象为空
    , mLogFunction(NULL) // 函数指针置空
{
    // 连接内部信号 logMessageReady 到外部接收器的槽函数
    // 使用 Qt::QueuedConnection 排队连接，确保线程安全
    connect(this, SIGNAL(logMessageReady(QString,int)), receiver, member, Qt::QueuedConnection);
}

// 写入日志消息
void QsLogging::FunctorDestination::write(const QString &message, QsLogging::Level level)
{
    // 如果设置了函数回调，则直接调用该函数
    if (mLogFunction)
        mLogFunction(message, level);

    // 如果日志级别大于 TraceLevel，则发射信号
    // 这里排除了 TraceLevel，可能是为了避免发送过多低级别的日志信号
    if (level > QsLogging::TraceLevel)
        // 发射 logMessageReady 信号，传递消息和日志级别
        emit logMessageReady(message, static_cast<int>(level));
}

// 检查目的地是否有效
bool QsLogging::FunctorDestination::isValid()
{
    // 对于函数对象目的地，它总是有效的
    return true;
}
