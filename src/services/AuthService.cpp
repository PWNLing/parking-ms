// services/AuthService.cpp
#include "AuthService.h"
#include "core/DatabaseManager.h"
#include "core/Md5.h"
#include "core/Logger.h"
#include "core/Session.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QRegularExpression>

bool AuthService::isValidPhone(const QString& phone) {
    static QRegularExpression re("^1[3-9]\\d{9}$");
    return re.match(phone).hasMatch();
}

bool AuthService::isValidUsername(const QString& u) {
    if (u.length() < 3) return false;
    static QRegularExpression re("^[A-Za-z0-9_]+$");
    return re.match(u).hasMatch();
}

bool AuthService::isValidPassword(const QString& p) {
    return p.length() >= 6;
}

bool AuthService::usernameExists(const QString& username) {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id FROM USER WHERE username = ?;");
    q.addBindValue(username);
    q.exec();
    return q.next();
}

std::optional<Session> AuthService::login(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) return std::nullopt;

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id, username, truename, telephone FROM USER "
              "WHERE username = ? AND password = ?;");
    q.addBindValue(username);
    q.addBindValue(Md5::hash(password));
    if (!q.exec() || !q.next()) {
        Logger::log(QtWarningMsg, QString("Login failed for user: %1").arg(username));
        return std::nullopt;
    }
    Session s;
    s.userId    = q.value(0).toInt();
    s.username  = q.value(1).toString();
    s.truename  = q.value(2).toString();
    s.telephone = q.value(3).toString();
    s.loginAt   = QDateTime::currentMSecsSinceEpoch();
    SessionManager::instance().set(s);
    Logger::log(QtInfoMsg, QString("Login OK: %1").arg(username));
    return s;
}

bool AuthService::registerUser(const QString& username, const QString& password,
                                const QString& truename, const QString& telephone,
                                QString* err) {
    if (!isValidUsername(username)) { if (err) *err = "账号需≥3位且仅含字母数字下划线"; return false; }
    if (!isValidPassword(password)) { if (err) *err = "密码至少6位"; return false; }
    if (truename.isEmpty())         { if (err) *err = "请填写真实姓名"; return false; }
    if (!isValidPhone(telephone))   { if (err) *err = "手机号格式不正确"; return false; }
    if (usernameExists(username))   { if (err) *err = "该账号已存在"; return false; }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("INSERT INTO USER(username, password, truename, telephone) VALUES (?, ?, ?, ?);");
    q.addBindValue(username);
    q.addBindValue(Md5::hash(password));
    q.addBindValue(truename);
    q.addBindValue(telephone);
    if (!q.exec()) {
        if (err) *err = QString("注册失败: %1").arg(q.lastError().text());
        return false;
    }
    Logger::log(QtInfoMsg, QString("Register OK: %1").arg(username));
    return true;
}

bool AuthService::changePassword(int userId, const QString& oldPwd,
                                  const QString& newPwd, QString* err) {
    if (!isValidPassword(newPwd)) { if (err) *err = "新密码至少6位"; return false; }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT password FROM USER WHERE id = ?;");
    q.addBindValue(userId);
    if (!q.exec() || !q.next()) {
        if (err) *err = "用户不存在";
        return false;
    }
    if (q.value(0).toString() != Md5::hash(oldPwd)) {
        if (err) *err = "旧密码错误";
        return false;
    }
    q.prepare("UPDATE USER SET password = ? WHERE id = ?;");
    q.addBindValue(Md5::hash(newPwd));
    q.addBindValue(userId);
    if (!q.exec()) {
        if (err) *err = QString("更新失败: %1").arg(q.lastError().text());
        return false;
    }
    Logger::log(QtInfoMsg, QString("Password changed: userId=%1").arg(userId));
    return true;
}
