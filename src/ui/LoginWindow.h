// ui/LoginWindow.h — 登录页（左图右内容）
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void onLogin();
    void onRegister();

private:
    QLineEdit* userEdit_ = nullptr;
    QLineEdit* pwdEdit_  = nullptr;
    QLabel*    errLabel_ = nullptr;

    void showError(const QString& msg);
};
