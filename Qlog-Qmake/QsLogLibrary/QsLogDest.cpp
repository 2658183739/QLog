#include "QsLogDest.h"
#include "QsLogDestConsole.h"
#include "QsLogDestFile.h"
#include "QsLogDestFunctor.h"
#include <QString>
#include <QScopedPointer>
#include <QtGlobal>

namespace QsLogging
{
// 使用虚函数确保子类的析构函数也会被调用
Destination::~Destination() {}

// 目的地工厂类，负责创建不同类型的日志目的地
DestinationPtr DestinationFactory::MakeFileDestination(const QString& filePath,
    LogRotationOption rotation, const MaxLogLines &linesToRotateAfter,
    const MaxOldLogCount &oldLogsToKeep)
{
//    // 用于处理日志文件轮转。
//    if (EnableLogRotation == rotation) {
//        // 创建一个基于行数的日志轮转策略
//        QScopedPointer<LineRotationStrategy> logRotation(new LineRotationStrategy);
//        // 设置最大行数
//        logRotation->setMaximumLines(linesToRotateAfter.lines);
//        // 设置要保留的旧日志文件数量
//        logRotation->setBackupCount(oldLogsToKeep.count);
//        // 创建并返回带有行数轮转策略的文件目的地
//        return DestinationPtr(new FileDestination(filePath, RotationStrategyPtr(logRotation.take())));
//    }
//    // 创建并返回带有空（不轮转）策略的文件目的地
//    return DestinationPtr(new FileDestination(filePath, RotationStrategyPtr(new NullRotationStrategy)));
}

// 创建一个将日志输出到调试控制台的目的地
DestinationPtr DestinationFactory::MakeDebugOutputDestination()
{
    return DestinationPtr(new DebugOutputDestination);
}

// 创建一个以函数作为回调的目的地
DestinationPtr DestinationFactory::MakeFunctorDestination(QsLogging::Destination::LogFunction f)
{
    return DestinationPtr(new FunctorDestination(f));
}

// 创建一个以 Qt 信号-槽作为回调的目的地
DestinationPtr DestinationFactory::MakeFunctorDestination(QObject *receiver, const char *member)
{
    return DestinationPtr(new FunctorDestination(receiver, member));
}
} // end namespace
