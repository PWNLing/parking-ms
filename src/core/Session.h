// core/Session.h — 登录会话单例
#pragma once
#include <QString>
#include <optional>
#include <QtGlobal>

/// 登录会话信息（内存，不持久化）
struct Session {
    int     userId   = 0;
    QString username;
    QString truename;
    QString telephone;
    qint64  loginAt  = 0;   // 登录时间戳 ms
};

class SessionManager {
public:
    static SessionManager& instance();

    void set(const Session& s);
    const std::optional<Session>& current() const { return session_; }
    void clear();
    bool isLoggedIn() const { return session_.has_value(); }

private:
    SessionManager() = default;
    std::optional<Session> session_;
};
