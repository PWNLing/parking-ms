// core/Logger.h — 日志器（qInstallMessageHandler + 按天滚动）
#pragma once
#include <QString>

class Logger {
public:
    /// 初始化日志系统，logDir 为日志目录
    static void init(const QString& logDir = "logs");

    /// Qt 消息处理回调（注册到 qInstallMessageHandler）
    static void handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);

    /// 直接写入一条日志
    static void log(QtMsgType type, const QString& msg);

private:
    Logger() = default;
    static QString logDir_;
    static QString currentLogFile();
};
