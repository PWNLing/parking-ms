// ui/AccountDialog.h — 账户管理（表格 CRUD）
#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QLabel>
class AccountDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountDialog(QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onUpdate();
    void onDelete();
    void onResetPwd();
    void onReload();

private:
    QTableWidget* table_ = nullptr;
    QLabel*       hintLabel_ = nullptr;

    void loadUsers();
    int  currentUserId();
};
