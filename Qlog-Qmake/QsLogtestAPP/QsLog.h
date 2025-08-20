#ifndef QSLOG_H
#define QSLOG_H

#include "QsLogLevel.h"
#include "QsLogDest.h"
#include <QDebug>
#include <QString>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>
#include <QThreadPool>
#include <QWaitCondition>
#include <atomic>

namespace QsLogging
{
class LoggerImpl;

// Logger 单例类
class Logger
{
public:
    // 获取 Logger 单例的静态方法
    static Logger& instance();
    // 销毁 Logger 单例的静态方法
    static void destroyInstance();
    // 析构函数
    ~Logger();

    // 强制同步写入所有待处理的日志。
    void flush();

    //添加一个日志消息目标。不能添加空指针。
    void addDestination(DestinationPtr destination);
    //设置日志级别，低于该级别的日志将被忽略。
    void setLoggingLevel(Level newLevel);
    //获取当前日志级别，默认级别为 INFO
    Level loggingLevel() const;
    //设置是否在日志消息中包含时间戳
    void setIncludeTimestamp(bool e);
    //获取是否包含时间戳，默认为 true。
    bool includeTimestamp() const;
    //设置是否在日志消息中包含日志级别
    void setIncludeLogLevel(bool l);
    //获取是否包含日志级别，默认为 true。
    bool includeLogLevel() const;

    //Helper 类，用于将流式日志重定向到 QDebug 并构建最终的日志消息。
    class Helper
    {
    public:
        // 接收日志级别
        explicit Helper(Level logLevel) :
            level(logLevel), qtDebug(new QDebug(&buffer)) {}
        // 负责将日志消息发送给 Logger
        ~Helper();
        // 获取 QDebug 流，用于写入日志内容
        QDebug& stream(){ return *qtDebug; }

    private:
        Level level;
        QString buffer;
        QSharedPointer<QDebug> qtDebug;
    };

private:
    // 构造函数私有，防止外部实例化
    Logger();
    // 禁用拷贝构造函数
    Logger(const Logger&);
    // 禁用赋值操作符
    Logger& operator=(const Logger&);

    LoggerImpl* d; // 指向实现类的指针
};

} // end namespace QsLogging

//日志宏定义：如果定义了 QS_LOG_LINE_NUMBERS，日志输出将包含文件和行号。
#ifndef QS_LOG_LINE_NUMBERS
#define QLOG_TRACE() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::TraceLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::TraceLevel).stream()
#define QLOG_DEBUG() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::DebugLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::DebugLevel).stream()
#define QLOG_INFO()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::InfoLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::InfoLevel).stream()
#define QLOG_WARN()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::WarnLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::WarnLevel).stream()
#define QLOG_ERROR() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::ErrorLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::ErrorLevel).stream()
#define QLOG_FATAL() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::FatalLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::FatalLevel).stream()
#else
// 定义了 QS_LOG_LINE_NUMBERS 的宏，包含文件和行号
#define QLOG_TRACE() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::TraceLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::TraceLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_DEBUG() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::DebugLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::DebugLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_INFO()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::InfoLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::InfoLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_WARN()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::WarnLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::WarnLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_ERROR() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::ErrorLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::ErrorLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_FATAL() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::FatalLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::FatalLevel).stream() << __FILE__ << '@' << __LINE__
#endif

#ifdef QS_LOG_DISABLE
#include "QsLogDisableForThisFile.h"
#endif

#endif // QSLOG_H
