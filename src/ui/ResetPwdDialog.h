// ui/ResetPwdDialog.h — 重置/修改密码对话框
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class ResetPwdDialog : public QDialog {
    Q_OBJECT
public:
    explicit ResetPwdDialog(int userId, bool isSelf = true, QWidget* parent = nullptr);

private slots:
    void onConfirm();

private:
    int         userId_;
    bool        isSelf_;   // true=改自己密码(需旧密码), false=管理员重置他人密码(无旧密码)
    QLineEdit*  oldEdit_   = nullptr;
    QLineEdit*  newEdit_   = nullptr;
    QLineEdit*  new2Edit_  = nullptr;
    QLabel*     errLabel_  = nullptr;
};
