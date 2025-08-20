#ifndef QSLOGDESTFUNCTOR_H
#define QSLOGDESTFUNCTOR_H

#include "QsLogDest.h"
#include <QObject>

namespace QsLogging
{
// 用于通过函数对象或信号槽机制处理日志消息
class FunctorDestination : public QObject, public Destination
{
    Q_OBJECT

public:
    // 接受一个函数对象作为日志处理函数
    explicit FunctorDestination(LogFunction f);
    // 接受一个 QObject 及其成员函数作为日志处理槽
    FunctorDestination(QObject *receiver, const char *member);

    // 实现基类的 write 纯虚函数
    // 在这里，它会调用传入的函数对象，或者发射一个信号
    void write(const QString &message, Level level) override;

    // 实现基类的 isValid 纯虚函数，检查目标是否有效
    bool isValid() override;

protected:
    // 信号：当有日志消息准备好时发射，可连接到任意槽
    Q_SIGNAL void logMessageReady(const QString &message, int level);

private:
    LogFunction mLogFunction; // 用于存储传入的函数对象
};
}

#endif // QSLOGDESTFUNCTOR_H
