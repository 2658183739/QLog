#ifndef QSLOGDEST_H
#define QSLOGDEST_H

#include "QsLogLevel.h"
#include <QSharedPointer>
#include <QtGlobal>
class QString;
class QObject;

// 根据编译模式定义共享库的导出/导入宏
// QSLOG_IS_SHARED_LIBRARY: 如果作为共享库构建，导出符号
#ifdef QSLOG_IS_SHARED_LIBRARY
#define QSLOG_SHARED_OBJECT Q_DECL_EXPORT
// QSLOG_IS_SHARED_LIBRARY_IMPORT: 如果作为共享库导入，导入符号
#elif QSLOG_IS_SHARED_LIBRARY_IMPORT
#define QSLOG_SHARED_OBJECT Q_DECL_IMPORT
// 否则，作为静态库或内部编译时，宏为空
#else
#define QSLOG_SHARED_OBJECT
#endif

namespace QsLogging
{

// 日志目标抽象基类
class QSLOG_SHARED_OBJECT Destination
{
public:
    // 函数指针类型定义，用于支持函数式日志目标
    typedef void (*LogFunction)(const QString &message, Level level);

public:
    // 虚析构函数
    virtual ~Destination();
    // 纯虚函数，用于将日志消息写入目标
    virtual void write(const QString& message, Level level) = 0;
    // 纯虚函数，用于检查目标是否有效
    virtual bool isValid() = 0;
};
// Destination 智能指针类型定义
typedef QSharedPointer<Destination> DestinationPtr;

// 日志轮转选项枚举
enum LogRotationOption
{
    DisableLogRotation = 0, // 禁用日志轮转
    EnableLogRotation  = 1  // 启用日志轮转
};

// 用于指定日志文件最大大小的结构体
struct QSLOG_SHARED_OBJECT MaxSizeBytes
{
    MaxSizeBytes() : size(0) {}
    explicit MaxSizeBytes(qint64 size_) : size(size_) {}
    qint64 size;
};

// 用于指定日志文件最大行数的结构体
struct QSLOG_SHARED_OBJECT MaxLogLines
{
    MaxLogLines() : lines(0) {}
    explicit MaxLogLines(int lines_) : lines(lines_) {}
    int lines;
};

// 用于指定保留旧日志文件最大数量的结构体
struct QSLOG_SHARED_OBJECT MaxOldLogCount
{
    MaxOldLogCount() : count(0) {}
    explicit MaxOldLogCount(int count_) : count(count_) {}
    int count;
};

// 日志目标工厂类，用于创建不同类型的日志目标
class QSLOG_SHARED_OBJECT DestinationFactory
{
public:
    // 创建文件日志目标的静态方法
    static DestinationPtr MakeFileDestination(const QString& filePath,
        LogRotationOption rotation = DisableLogRotation,
        const MaxLogLines &linesToRotateAfter = MaxLogLines(),
        const MaxOldLogCount &oldLogsToKeep = MaxOldLogCount());
    // 创建调试输出日志目标的静态方法
    static DestinationPtr MakeDebugOutputDestination();
    // 创建基于函数指针的日志目标的静态方法
    static DestinationPtr MakeFunctorDestination(Destination::LogFunction f);
    // 创建基于 QObject 成员函数的日志目标的静态方法
    static DestinationPtr MakeFunctorDestination(QObject *receiver, const char *member);
};

} // end namespace QsLogging

#endif // QSLOGDEST_H
