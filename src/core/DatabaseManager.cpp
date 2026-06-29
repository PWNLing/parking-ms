// core/DatabaseManager.cpp
#include "DatabaseManager.h"
#include "Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QThread>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDir>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

QString DatabaseManager::threadConnName() const {
    // 每个线程独立连接名
    return QString("conn_%1").arg(reinterpret_cast<quintptr>(QThread::currentThread()));
}

QSqlDatabase DatabaseManager::connection() {
    QMutexLocker locker(&mutex_);
    QString name = threadConnName();

    if (QSqlDatabase::contains(name)) {
        auto db = QSqlDatabase::database(name);
        if (db.isOpen()) return db;
        db.open();
        return db;
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(dbPath_);
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP=1;");
    if (!db.open()) {
        Logger::log(QtCriticalMsg, QString("DB open failed: %1").arg(db.lastError().text()));
        return db;
    }
    // 启用外键
    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON;");
    q.exec("PRAGMA journal_mode = WAL;");
    return db;
}

void DatabaseManager::releaseConnection() {
    QMutexLocker locker(&mutex_);
    QString name = threadConnName();
    if (QSqlDatabase::contains(name)) {
        auto db = QSqlDatabase::database(name);
        if (db.isOpen()) db.close();
        QSqlDatabase::removeDatabase(name);
    }
}

bool DatabaseManager::createWithSchema(const QString& dbPath, const QString& schemaResource) {
    dbPath_ = dbPath;

    // 确保目录存在
    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());

    // 加载 schema SQL（从 Qt 资源）
    QFile f(schemaResource);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::log(QtCriticalMsg, QString("Cannot open schema resource: %1").arg(schemaResource));
        return false;
    }
    QString schemaSql = QTextStream(&f).readAll();
    f.close();

    // 创建临时连接执行 schema（用作用域确保 query 析构后再 removeDatabase）
    bool ok = false;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", "schema_init");
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            Logger::log(QtCriticalMsg, QString("DB create failed: %1").arg(db.lastError().text()));
        } else {
            ok = execSchema(db, schemaSql);
            db.close();
        }
    }  // db 与内部 QSqlQuery 在此析构
    QSqlDatabase::removeDatabase("schema_init");

    if (ok) Logger::log(QtInfoMsg, QString("Database created with schema: %1").arg(dbPath));
    return ok;
}

bool DatabaseManager::execSchema(QSqlDatabase& db, const QString& schemaSql) {
    // 移除 -- 注释行
    QString cleaned = schemaSql;
    cleaned.remove(QRegularExpression("--[^\\n]*"));

    // 智能分割：尊重 CREATE TRIGGER ... BEGIN ... END; 块（块内的 ; 不分割）
    QStringList stmts;
    QString buf;
    bool inBlock = false;
    for (int i = 0; i < cleaned.size(); ++i) {
        QChar c = cleaned[i];
        buf += c;
        if (!inBlock) {
            if (c == ';') {
                QString s = buf.trimmed();
                if (!s.isEmpty() && s != ";") stmts << s;
                buf.clear();
            } else if (buf.endsWith("BEGIN", Qt::CaseSensitive)) {
                int idx = buf.size() - 5;
                if (idx == 0 || buf[idx - 1].isSpace()) {
                    inBlock = true;
                }
            }
        } else {
            // 在触发器块内：仅当遇到 END; 时结束
            if (c == ';') {
                QString before = buf.left(buf.size() - 1).trimmed();
                if (before.endsWith("END", Qt::CaseSensitive)) {
                    inBlock = false;
                    QString s = buf.trimmed();
                    if (!s.isEmpty()) stmts << s;
                    buf.clear();
                }
            }
        }
    }
    QString rem = buf.trimmed();
    if (!rem.isEmpty()) stmts << rem;

    Logger::log(QtInfoMsg, QString("Schema: executing %1 statements").arg(stmts.size()));
    QSqlQuery q(db);
    for (const QString& s : stmts) {
        if (!q.exec(s)) {
            Logger::log(QtCriticalMsg, QString("Schema stmt failed: %1 | err: %2")
                        .arg(s.left(80)).arg(q.lastError().text()));
            return false;
        }
    }
    return true;
}

bool DatabaseManager::validate(const QString& dbPath) {
    if (!QFile::exists(dbPath)) return false;
    // 用作用域确保 QSqlDatabase/QSqlQuery 在 removeDatabase 前析构，避免 "connection still in use" 警告
    bool ok = false;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", "validate");
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            QSqlDatabase::removeDatabase("validate");
            return false;
        }
        QStringList required = {"USER", "PARKING", "CAR", "reservations"};
        QSqlQuery q(db);
        ok = true;
        for (const auto& t : required) {
            q.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?;");
            q.addBindValue(t);
            if (!q.exec() || !q.next()) {
                ok = false;
                break;
            }
        }
        db.close();
    }  // db 与 q 在此析构
    QSqlDatabase::removeDatabase("validate");
    return ok;
}

bool DatabaseManager::seedDefault(const QString& parkingName, int capacity, double fee,
                                  const QString& adminUser, const QString& adminPwdMd5,
                                  const QString& adminTrueName, const QString& adminPhone) {
    auto db = connection();
    if (!db.transaction()) {
        Logger::log(QtCriticalMsg, "seedDefault: BEGIN failed");
        return false;
    }
    QSqlQuery q(db);
    // 插入停车场
    q.prepare("INSERT INTO PARKING(P_name, P_all_count, P_fee) VALUES (?, ?, ?);");
    q.addBindValue(parkingName);
    q.addBindValue(capacity);
    q.addBindValue(fee);
    if (!q.exec()) {
        Logger::log(QtCriticalMsg, QString("Seed PARKING failed: %1").arg(q.lastError().text()));
        db.rollback();
        return false;
    }
    // 插入默认管理员
    q.prepare("INSERT INTO USER(username, password, truename, telephone) VALUES (?, ?, ?, ?);");
    q.addBindValue(adminUser);
    q.addBindValue(adminPwdMd5);
    q.addBindValue(adminTrueName);
    q.addBindValue(adminPhone);
    if (!q.exec()) {
        Logger::log(QtCriticalMsg, QString("Seed USER failed: %1").arg(q.lastError().text()));
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        db.rollback();
        return false;
    }
    Logger::log(QtInfoMsg, QString("Seed OK: parking=%1 cap=%2 fee=%3 admin=%4")
                .arg(parkingName).arg(capacity).arg(fee).arg(adminUser));
    return true;
}

bool DatabaseManager::transaction(const std::function<bool(QSqlDatabase&)>& fn) {
    auto db = connection();
    if (!db.isOpen()) return false;
    if (!db.transaction()) {
        Logger::log(QtWarningMsg, "BEGIN transaction failed");
        return false;
    }
    bool ok = false;
    try {
        ok = fn(db);
    } catch (...) {
        ok = false;
    }
    if (ok) {
        if (!db.commit()) {
            Logger::log(QtCriticalMsg, "COMMIT failed");
            db.rollback();
            return false;
        }
        return true;
    } else {
        db.rollback();
        return false;
    }
}
