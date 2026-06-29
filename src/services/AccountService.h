// services/AccountService.h — 账户管理 CRUD
#pragma once
#include <QString>
#include <vector>
#include "models/UserRow.h"

class AccountService {
public:
    /// 列出全部账户
    std::vector<UserRow> listAll();

    /// 新增账户（password 为明文，内部 MD5）
    bool addUser(const QString& username, const QString& plainPwd,
                 const QString& truename, const QString& telephone, QString* err);

    /// 修改账户（姓名/手机号，不改密码）
    bool updateUser(int id, const QString& truename, const QString& telephone, QString* err);

    /// 重置密码
    bool resetPassword(int id, const QString& newPlainPwd, QString* err);

    /// 删除账户；禁止删除 currentUserId 自己
    bool deleteUser(int id, int currentUserId, QString* err);

    // 校验工具（与 AuthService 共用）
    static bool isValidPhone(const QString& p);
    static bool isValidUsername(const QString& u);
    static bool isValidPassword(const QString& p);
};
