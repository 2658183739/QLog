#ifndef QSLOGDESTCONSOLE_H
#define QSLOGDESTCONSOLE_H

#include "QsLogDest.h"
#include <QString>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QtGlobal>

namespace QsLogging
{
    // 这是一个将日志消息输出到调试控制台的日志目标
    class DebugOutputDestination : public Destination
    {
    public:
        // 实现基类的 write 纯虚函数
        // 将日志消息写入 QDebug，通常会输出到应用程序的“调试”面板或标准错误流
        void write(const QString& message, Level level) override;
        // 实现基类的 isValid 纯虚函数
        // 检查目标是否有效，对于调试输出，它总是有效的
        bool isValid() override;
    };
}

#endif // QSLOGDESTCONSOLE_H
