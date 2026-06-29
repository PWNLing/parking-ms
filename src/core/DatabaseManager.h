// core/DatabaseManager.h — SQLite 连接池 + schema 执行 + 事务包装
#pragma once
#include <QSqlDatabase>
#include <QString>
#include <functional>
#include <QMutex>

struct ParkingInfo;
struct AdminAccount;

class DatabaseManager {
public:
    static DatabaseManager& instance();

    /// 创建数据库文件并执行 schema.sql（从 Qt 资源加载）
    bool createWithSchema(const QString& dbPath, const QString& schemaResource);

    /// 验证数据库文件有效性（可打开且包含必要表）
    bool validate(const QString& dbPath);

    /// 写入种子数据：停车场 + 默认管理员
    bool seedDefault(const QString& parkingName, int capacity, double fee,
                     const QString& adminUser, const QString& adminPwdMd5,
                     const QString& adminTrueName, const QString& adminPhone);

    /// 设置当前数据库路径（运行时）
    void setDbPath(const QString& path) { dbPath_ = path; }
    QString dbPath() const { return dbPath_; }

    /// 获取当前线程的数据库连接（线程安全，按线程名缓存连接）
    QSqlDatabase connection();

    /// 释放当前线程的连接
    void releaseConnection();

    /// 事务包装：fn 返回 true 提交，false 回滚
    bool transaction(const std::function<bool(QSqlDatabase&)>& fn);

private:
    DatabaseManager() = default;
    QString dbPath_;
    QMutex  mutex_;
    int     connCounter_ = 0;

    QString threadConnName() const;
    bool    execSchema(QSqlDatabase& db, const QString& schemaSql);
};
