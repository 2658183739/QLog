#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QVector>
#include <atomic>
#include <QElapsedTimer>
#include <thread>
#include "QsLog.h"
#include "QsLogDestFile.h"

// 使用线程安全的原子计数器，避免竞态条件
std::atomic<long long int> count(0);
// 使用原子布尔变量作为停止信号
std::atomic<bool> stopRequested(false);

// 日志生成函数，模拟多线程并发写入
void logGenerator(int threadId)
{
    for(int i = 0;i < 1000;i++)
    {
        // 定期检查是否收到停止信号，每1000次迭代检查一次
        if (i % 100 == 0) {
            if (stopRequested.load()) {
                qDebug() << "Thread " << threadId << ": Received stop signal, exiting gracefully.";
                break; // 收到停止信号，退出循环
            }
        }

        // 交替使用不同的日志级别生成日志
        if (i % 5 == 0) {
            QLOG_TRACE() << "Thread " << threadId << ": This is a TRACE message number " << i;
        } else if (i % 5 == 1) {
            QLOG_DEBUG() << "Thread " << threadId << ": This is a DEBUG message number " << i;
        } else if (i % 5 == 2) {
            QLOG_INFO() << "Thread " << threadId << ": This is an INFO message number " << i;
        } else if (i % 5 == 3) {
            QLOG_WARN() << "Thread " << threadId << ": This is a WARNING message number " << i;
        } else {
            QLOG_ERROR() << "Thread " << threadId << ": This is an ERROR message number " << i;
        }

        count++; // 每次成功写入日志，计数加1
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 确保日志目录存在
    QDir logDir("logs");
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    // 获取logger单例
    QsLogging::Logger& logger = QsLogging::Logger::instance();
    // 设置日志级别为Trace，显示所有日志
    logger.setLoggingLevel(QsLogging::TraceLevel);

    // 创建控制台输出目标
    QsLogging::DestinationPtr debugDestination(
        QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination);

    // 创建SQLite数据库文件输出目标
    const QString dbLogPath = logDir.absoluteFilePath("log.db");
    QsLogging::DestinationPtr dbFileDestination(
        new QsLogging::DatabaseDestination(dbLogPath)
    );
    logger.addDestination(dbFileDestination);

    QLOG_INFO() << "日志系统已成功初始化。开始为期10秒的高强度多线程测试...";

    // 启动一个计时器
    QElapsedTimer timer;
    timer.start();

    // 使用QtConcurrent启动10个并发任务，模拟多线程写入
    QVector<QFuture<void>> futures;
    for (int i = 1; i <= 10; ++i) {
        QFuture<void> future = QtConcurrent::run(logGenerator, i);
        futures.append(future);
    }

    // 循环检查时间，直到10秒时间到
    const int TIME_LIMIT_MS = 10 * 1000; // 10秒转换为毫秒
    while (timer.elapsed() < TIME_LIMIT_MS) {
        // 短暂休眠以避免CPU空转，并让其他线程有机会运行
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 计时器到期，设置停止信号
    stopRequested.store(true);
    qDebug() << "Time limit of 10 seconds reached, sending stop signal to all threads.";

    // 等待所有任务完成（确保线程接收到停止信号后优雅退出）
    for (QFuture<void>& future : futures) {
        future.waitForFinished();
    }

    // 在程序退出前，强制将所有待处理日志写入文件
    logger.flush();

    QLOG_INFO() << "测试已完成。总日志条数: " << count.load();

    return a.exec();
}
