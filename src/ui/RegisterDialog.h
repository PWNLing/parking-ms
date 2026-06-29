// ui/RegisterDialog.h — 注册对话框
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class RegisterDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget* parent = nullptr);
    QString username() const { return userEdit_->text().trimmed(); }

private slots:
    void onRegister();

private:
    QLineEdit* userEdit_   = nullptr;
    QLineEdit* pwdEdit_    = nullptr;
    QLineEdit* pwd2Edit_   = nullptr;
    QLineEdit* nameEdit_   = nullptr;
    QLineEdit* phoneEdit_  = nullptr;
    QLabel*    errLabel_   = nullptr;

    void showError(const QString& msg);
};
