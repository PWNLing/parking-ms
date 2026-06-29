// services/AuthService.h — 注册/登录/改密
#pragma once
#include <QString>
#include <optional>
#include "core/Session.h"

class AuthService {
public:
    /// 登录：成功返回 Session，失败返回 nullopt
    std::optional<Session> login(const QString& username, const QString& password);

    /// 注册新用户；err 返回错误描述
    bool registerUser(const QString& username, const QString& password,
                      const QString& truename, const QString& telephone, QString* err);

    /// 修改密码（校验旧密码）
    bool changePassword(int userId, const QString& oldPwd,
                        const QString& newPwd, QString* err);

    /// 用户名是否存在
    bool usernameExists(const QString& username);

    /// 手机号校验
    static bool isValidPhone(const QString& phone);

    /// 用户名校验
    static bool isValidUsername(const QString& u);

    /// 密码强度校验
    static bool isValidPassword(const QString& p);
};
