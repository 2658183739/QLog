#ifndef QSLOGDISABLEFORTHISFILE_H
#define QSLOGDISABLEFORTHISFILE_H

#include <QtDebug>

// 首先取消定义所有日志宏，以防止重复定义警告
#undef QLOG_TRACE
#undef QLOG_DEBUG
#undef QLOG_INFO
#undef QLOG_WARN
#undef QLOG_ERROR
#undef QLOG_FATAL

// 重新定义所有日志宏为空操作
// QLOG_TRACE() 宏现在被定义为一个无操作的 if 语句。
// 编译器会优化掉 'if (1) {}' 部分，从而使得 qDebug() 永远不会被调用，
// 从而有效地禁用了日志输出，并且不会产生任何运行时开销。
#define QLOG_TRACE() if (1) {} else qDebug()
#define QLOG_DEBUG() if (1) {} else qDebug()
#define QLOG_INFO()  if (1) {} else qDebug()
#define QLOG_WARN()  if (1) {} else qDebug()
#define QLOG_ERROR() if (1) {} else qDebug()
#define QLOG_FATAL() if (1) {} else qDebug()

#endif // QSLOGDISABLEFORTHISFILE_H
