// ui/RegisterDialog.cpp
#include "RegisterDialog.h"
#include "services/AuthService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

RegisterDialog::RegisterDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("注册管理员账户");
    setFixedSize(380, 380);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("创建新账户", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    userEdit_  = new QLineEdit(this);  userEdit_->setPlaceholderText("≥3位字母数字下划线");
    pwdEdit_   = new QLineEdit(this);  pwdEdit_->setEchoMode(QLineEdit::Password); pwdEdit_->setPlaceholderText("≥6位");
    pwd2Edit_  = new QLineEdit(this);  pwd2Edit_->setEchoMode(QLineEdit::Password);
    nameEdit_  = new QLineEdit(this);
    phoneEdit_ = new QLineEdit(this);  phoneEdit_->setPlaceholderText("11位手机号");
    form->addRow("账号:",   userEdit_);
    form->addRow("密码:",   pwdEdit_);
    form->addRow("确认密码:", pwd2Edit_);
    form->addRow("真实姓名:", nameEdit_);
    form->addRow("手机号:",  phoneEdit_);
    root->addLayout(form);

    errLabel_ = new QLabel(this);
    errLabel_->setObjectName("errorLabel");
    errLabel_->setWordWrap(true);
    errLabel_->hide();
    root->addWidget(errLabel_);

    root->addStretch();

    auto* btnRow = new QHBoxLayout;
    auto* btnCancel = new QPushButton("取消", this);
    auto* btnOk = new QPushButton("注册", this);
    btnOk->setObjectName("primaryBtn");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &RegisterDialog::onRegister);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnOk);
    root->addLayout(btnRow);
}

void RegisterDialog::showError(const QString& msg) {
    errLabel_->setText(msg);
    errLabel_->show();
}

void RegisterDialog::onRegister() {
    QString u = userEdit_->text().trimmed();
    QString p = pwdEdit_->text();
    QString p2 = pwd2Edit_->text();
    QString n = nameEdit_->text().trimmed();
    QString t = phoneEdit_->text().trimmed();

    if (p != p2) {
        showError("两次密码不一致");
        return;
    }
    QString err;
    if (AuthService().registerUser(u, p, n, t, &err)) {
        accept();
    } else {
        showError(err);
    }
}
