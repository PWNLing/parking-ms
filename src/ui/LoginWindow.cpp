// ui/LoginWindow.cpp
#include "LoginWindow.h"
#include "services/AuthService.h"
#include "ui/RegisterDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>

LoginWindow::LoginWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("停车场管理系统 - 登录");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    setFixedSize(820, 480);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 左侧 banner
    auto* banner = new QLabel(this);
    banner->setObjectName("loginBanner");
    banner->setMinimumWidth(380);
    banner->setPixmap(QPixmap(":/icons/login_bg.svg"));
    banner->setScaledContents(true);
    root->addWidget(banner);

    // 右侧表单
    auto* right = new QWidget(this);
    right->setMinimumWidth(440);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setSpacing(16);
    rightLayout->setContentsMargins(40, 40, 40, 40);

    auto* title = new QLabel("管理员登录", right);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(12);
    userEdit_ = new QLineEdit(right);
    userEdit_->setPlaceholderText("账号");
    userEdit_->setText("admin");   // 默认账号
    pwdEdit_  = new QLineEdit(right);
    pwdEdit_->setEchoMode(QLineEdit::Password);
    pwdEdit_->setPlaceholderText("密码");
    pwdEdit_->setText("123456");  // 默认密码
    form->addRow("账号:", userEdit_);
    form->addRow("密码:", pwdEdit_);
    rightLayout->addLayout(form);

    errLabel_ = new QLabel(right);
    errLabel_->setObjectName("errorLabel");
    errLabel_->setWordWrap(true);
    errLabel_->hide();
    rightLayout->addWidget(errLabel_);

    rightLayout->addStretch();

    // 按钮行
    auto* btnRow = new QHBoxLayout;
    auto* btnRegister = new QPushButton("注册", right);
    auto* btnLogin = new QPushButton("登录", right);
    btnLogin->setObjectName("primaryBtn");
    btnRow->addStretch();
    btnRow->addWidget(btnRegister);
    btnRow->addWidget(btnLogin);
    rightLayout->addLayout(btnRow);

    // 回车连通登录
    connect(pwdEdit_, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(userEdit_, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegister);

    root->addWidget(right);
}

void LoginWindow::showError(const QString& msg) {
    errLabel_->setText(msg);
    errLabel_->show();
    pwdEdit_->clear();
}

void LoginWindow::onLogin() {
    QString u = userEdit_->text().trimmed();
    QString p = pwdEdit_->text();
    if (u.isEmpty() || p.isEmpty()) {
        showError("请输入账号和密码");
        return;
    }
    if (AuthService().login(u, p)) {
        accept();
    } else {
        showError("账号或密码错误");
    }
}

void LoginWindow::onRegister() {
    RegisterDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        userEdit_->setText(dlg.username());
        pwdEdit_->clear();
        pwdEdit_->setFocus();
    }
}
