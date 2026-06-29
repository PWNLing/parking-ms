// models/UserRow.h — 账户表行结构
#pragma once
#include <QString>
#include <QMetaType>

struct UserRow {
    int     id = 0;
    QString username;
    QString passwordHash;   // MD5 密文（表格只读显示）
    QString truename;
    QString telephone;
};

Q_DECLARE_METATYPE(UserRow)
