#include "QsLogDestConsole.h"
#include <QDebug>

namespace QsLogging
{

//该类是 QsLog 框架的一个日志目的地，用于将日志消息直接输出到 Qt 的调试流。
void DebugOutputDestination::write(const QString& message, Level)
{
    // 使用 Qt 的 qDebug() 函数将日志消息写入调试流
    // 这里的 Level 参数未被使用，因为所有级别的消息都将被统一输出。
    qDebug() << message;
}


//@brief 检查目的地是否有效。
//@return bool 始终返回 true，因为调试输出目的地总是可用的。

bool DebugOutputDestination::isValid()
{
    // 对于调试输出，它总是有效的
    return true;
}
} // end namespace
