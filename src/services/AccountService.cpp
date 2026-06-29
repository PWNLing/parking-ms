// services/AccountService.cpp
#include "AccountService.h"
#include "core/DatabaseManager.h"
#include "core/Md5.h"
#include "core/Logger.h"
#include "services/AuthService.h"
#include <QSqlQuery>
#include <QSqlError>

bool AccountService::isValidPhone(const QString& p)    { return AuthService::isValidPhone(p); }
bool AccountService::isValidUsername(const QString& u) { return AuthService::isValidUsername(u); }
bool AccountService::isValidPassword(const QString& p) { return AuthService::isValidPassword(p); }

std::vector<UserRow> AccountService::listAll() {
    std::vector<UserRow> out;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.exec("SELECT id, username, password, truename, telephone FROM USER ORDER BY id;");
    while (q.next()) {
        UserRow r;
        r.id            = q.value(0).toInt();
        r.username      = q.value(1).toString();
        r.passwordHash  = q.value(2).toString();
        r.truename      = q.value(3).toString();
        r.telephone     = q.value(4).toString();
        out.push_back(r);
    }
    return out;
}

bool AccountService::addUser(const QString& username, const QString& plainPwd,
                              const QString& truename, const QString& telephone, QString* err) {
    if (!isValidUsername(username)) { if (err) *err = "账号需≥3位且仅含字母数字下划线"; return false; }
    if (!isValidPassword(plainPwd)) { if (err) *err = "密码至少6位"; return false; }
    if (truename.isEmpty())         { if (err) *err = "请填写真实姓名"; return false; }
    if (!isValidPhone(telephone))   { if (err) *err = "手机号必须为11位且以1开头"; return false; }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("INSERT INTO USER(username, password, truename, telephone) VALUES (?, ?, ?, ?);");
    q.addBindValue(username);
    q.addBindValue(Md5::hash(plainPwd));
    q.addBindValue(truename);
    q.addBindValue(telephone);
    if (!q.exec()) {
        QString e = q.lastError().text();
        if (e.contains("UNIQUE", Qt::CaseInsensitive)) {
            if (err) *err = "该账号已存在";
        } else {
            if (err) *err = QString("添加失败: %1").arg(e);
        }
        return false;
    }
    Logger::log(QtInfoMsg, QString("User added: %1").arg(username));
    return true;
}

bool AccountService::updateUser(int id, const QString& truename, const QString& telephone,
                                 QString* err) {
    if (truename.isEmpty())       { if (err) *err = "请填写真实姓名"; return false; }
    if (!isValidPhone(telephone)) { if (err) *err = "手机号必须为11位且以1开头"; return false; }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("UPDATE USER SET truename = ?, telephone = ? WHERE id = ?;");
    q.addBindValue(truename);
    q.addBindValue(telephone);
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = QString("修改失败: %1").arg(q.lastError().text());
        return false;
    }
    return true;
}

bool AccountService::resetPassword(int id, const QString& newPlainPwd, QString* err) {
    if (!isValidPassword(newPlainPwd)) { if (err) *err = "密码至少6位"; return false; }
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("UPDATE USER SET password = ? WHERE id = ?;");
    q.addBindValue(Md5::hash(newPlainPwd));
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = QString("重置失败: %1").arg(q.lastError().text());
        return false;
    }
    Logger::log(QtInfoMsg, QString("Password reset: userId=%1").arg(id));
    return true;
}

bool AccountService::deleteUser(int id, int currentUserId, QString* err) {
    if (id == currentUserId) {
        if (err) *err = "不能删除当前登录账户";
        return false;
    }
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("DELETE FROM USER WHERE id = ?;");
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = QString("删除失败: %1").arg(q.lastError().text());
        return false;
    }
    Logger::log(QtInfoMsg, QString("User deleted: id=%1").arg(id));
    return true;
}
