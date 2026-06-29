// ui/RoleSelectDialog.h — 启动角色选择（管理员/用户）
#pragma once
#include <QDialog>

class RoleSelectDialog : public QDialog {
    Q_OBJECT
public:
    enum Role { Admin, User, Cancelled };
    explicit RoleSelectDialog(QWidget* parent = nullptr);
    Role selectedRole() const { return role_; }

private slots:
    void onAdminClicked();
    void onUserClicked();

private:
    Role role_ = Cancelled;
};
