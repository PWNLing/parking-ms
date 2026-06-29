// core/Logger.cpp
#include "Logger.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

QString Logger::logDir_ = "logs";

void Logger::init(const QString& logDir) {
    logDir_ = logDir;
    QDir().mkpath(logDir_);
    qInstallMessageHandler(&Logger::handler);
    log(QtInfoMsg, "=== ParkingMS Logger initialized ===");
}

QString Logger::currentLogFile() {
    QString date = QDateTime::currentDateTime().toString("yyyyMMdd");
    return logDir_ + "/parking_ms_" + date + ".log";
}

void Logger::log(QtMsgType type, const QString& msg) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG";   break;
        case QtInfoMsg:     level = "INFO ";   break;
        case QtWarningMsg:  level = "WARN ";   break;
        case QtCriticalMsg: level = "ERROR";   break;
        case QtFatalMsg:    level = "FATAL";   break;
        default:            level = "?????";   break;
    }

    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString line = QString("[%1] [%2] %3").arg(ts, level, msg);

    QFile f(currentLogFile());
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << line << "\n";
        f.close();
    }

#ifdef QT_DEBUG
    // Debug 构建同时输出到控制台
    fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
    fflush(stderr);
#endif
}

void Logger::handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    Q_UNUSED(ctx);
    // 移除 Qt 默认的类别前缀，保留纯消息
    QString clean = msg;
    log(type, clean);
}
