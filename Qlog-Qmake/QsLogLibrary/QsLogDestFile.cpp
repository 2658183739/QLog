#include "QsLogDestFile.h"
#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>

namespace QsLogging
{

// 将日志级别转换为整型，便于数据库存储。
int levelToInt(Level level) {
    return static_cast<int>(level);
}

// 初始化数据库连接和表
DatabaseDestination::DatabaseDestination(const QString& dbFilePath)
    : m_isDbValid(false)
{
    initDatabase(dbFilePath);
}

// 关闭数据库连接
DatabaseDestination::~DatabaseDestination()
{
    if (m_db.isOpen()) {
        m_db.close();
        qDebug() << "QsLog: SQLite database connection closed.";
    }
}

// 初始化数据库连接并创建表
void DatabaseDestination::initDatabase(const QString& dbFilePath)
{
    // 如果数据库文件已存在，则不会覆盖
    m_db = QSqlDatabase::addDatabase("QSQLITE", "log_connection");
    m_db.setDatabaseName(dbFilePath);

    if (!m_db.open()) {
        qWarning() << "QsLog: Failed to open SQLite database:" << m_db.lastError().text();
        m_isDbValid = false;
        return;
    }

    qDebug() << "QsLog: Successfully connected to SQLite database at" << dbFilePath;

    QSqlQuery createTableQuery(m_db);
    QString createTableSql = "CREATE TABLE IF NOT EXISTS log_entries ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "timestamp TEXT NOT NULL, "
                             "level INTEGER NOT NULL, "
                             "message TEXT NOT NULL"
                             ");";

    if (!createTableQuery.exec(createTableSql)) {
        qWarning() << "QsLog: Failed to create log_entries table:" << createTableQuery.lastError().text();
        m_db.close();
        m_isDbValid = false;
        return;
    }

    // 预处理插入查询，以提高性能
    m_query = QSqlQuery(m_db);
    m_query.prepare("INSERT INTO log_entries (timestamp, level, message) VALUES (:timestamp, :level, :message)");

    m_isDbValid = true;
}

// 写入日志到数据库
void DatabaseDestination::write(const QString& message, Level level)
{
    if (!m_isDbValid) {
        return;
    }

    // 使用事务以提高写入性能
    m_db.transaction();

    m_query.bindValue(":timestamp", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    m_query.bindValue(":level", levelToInt(level));
    m_query.bindValue(":message", message);

    if (!m_query.exec()) {
        qWarning() << "QsLog: Failed to insert log entry:" << m_query.lastError().text();
        m_db.rollback();
    } else {
        m_db.commit();
    }
}

// 检查数据库连接是否有效
bool DatabaseDestination::isValid()
{
    return m_isDbValid;
}

} // end namespace
