// ui/ResetPwdDialog.cpp
#include "ResetPwdDialog.h"
#include "services/AuthService.h"
#include "services/AccountService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

ResetPwdDialog::ResetPwdDialog(int userId, bool isSelf, QWidget* parent)
    : QDialog(parent), userId_(userId), isSelf_(isSelf) {
    setWindowTitle(isSelf ? "修改密码" : "重置密码");
    setFixedSize(360, isSelf ? 260 : 200);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(20, 20, 20, 20);

    auto* title = new QLabel(isSelf ? "修改我的密码" : "重置用户密码", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    if (isSelf) {
        oldEdit_ = new QLineEdit(this);
        oldEdit_->setEchoMode(QLineEdit::Password);
        form->addRow("旧密码:", oldEdit_);
    }
    newEdit_  = new QLineEdit(this); newEdit_->setEchoMode(QLineEdit::Password);
    new2Edit_ = new QLineEdit(this); new2Edit_->setEchoMode(QLineEdit::Password);
    form->addRow("新密码:", newEdit_);
    form->addRow("确认新密码:", new2Edit_);
    root->addLayout(form);

    errLabel_ = new QLabel(this);
    errLabel_->setObjectName("errorLabel");
    errLabel_->setWordWrap(true);
    errLabel_->hide();
    root->addWidget(errLabel_);

    root->addStretch();

    auto* btnRow = new QHBoxLayout;
    auto* btnCancel = new QPushButton("取消", this);
    auto* btnOk = new QPushButton("确认", this);
    btnOk->setObjectName("primaryBtn");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &ResetPwdDialog::onConfirm);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnOk);
    root->addLayout(btnRow);
}

void ResetPwdDialog::onConfirm() {
    QString n1 = newEdit_->text();
    QString n2 = new2Edit_->text();
    if (n1 != n2) {
        errLabel_->setText("两次新密码不一致");
        errLabel_->show();
        return;
    }
    QString err;
    bool ok;
    if (isSelf_) {
        ok = AuthService().changePassword(userId_, oldEdit_->text(), n1, &err);
    } else {
        ok = AccountService().resetPassword(userId_, n1, &err);
    }
    if (ok) {
        accept();
    } else {
        errLabel_->setText(err);
        errLabel_->show();
    }
}
