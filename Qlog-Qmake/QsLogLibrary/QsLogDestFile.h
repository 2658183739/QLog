#ifndef QSLOGDESTFILE_H
#define QSLOGDESTFILE_H

#include "QsLogDest.h"
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QSqlQuery>

namespace QsLogging
{


//将日志信息写入 SQLite 数据库的日志目的地。
class DatabaseDestination : public Destination
{
public:
    // 构造函数，需要一个数据库文件路径
    explicit DatabaseDestination(const QString& dbFilePath);
    // 析构函数，用于关闭数据库连接
    ~DatabaseDestination();

    // 实现基类的 write 纯虚函数，将日志消息写入数据库
    void write(const QString& message, Level level) override;
    // 实现基类的 isValid 纯虚函数，检查数据库连接是否有效
    bool isValid() override;

private:
    QSqlDatabase m_db;      // 数据库连接对象
    bool m_isDbValid;       // 标记数据库连接是否有效
    QSqlQuery m_query;      // 用于优化插入操作的预处理查询

    // 初始化数据库连接并创建表的私有方法
    void initDatabase(const QString& dbFilePath);
};

// DatabaseDestination 智能指针类型定义
typedef QSharedPointer<DatabaseDestination> DatabaseDestinationPtr;

}

#endif // QSLOGDESTFILE_H
