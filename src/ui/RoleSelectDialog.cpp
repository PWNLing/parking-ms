// ui/RoleSelectDialog.cpp
#include "RoleSelectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>

RoleSelectDialog::RoleSelectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("停车场管理系统 - 角色选择");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    setFixedSize(440, 320);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(20);
    root->setContentsMargins(30, 30, 30, 30);

    auto* title = new QLabel("请选择角色", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* subtitle = new QLabel("Smart Parking Management System", this);
    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);

    root->addStretch();

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(20);

    auto* btnAdmin = new QPushButton(QIcon(":/icons/user.svg"), "管理员模式", this);
    btnAdmin->setObjectName("primaryBtn");
    btnAdmin->setMinimumHeight(60);
    connect(btnAdmin, &QPushButton::clicked, this, &RoleSelectDialog::onAdminClicked);

    auto* btnUser = new QPushButton(QIcon(":/icons/parking.svg"), "用户模式", this);
    btnUser->setObjectName("primaryBtn");
    btnUser->setMinimumHeight(60);
    connect(btnUser, &QPushButton::clicked, this, &RoleSelectDialog::onUserClicked);

    btnRow->addWidget(btnAdmin);
    btnRow->addWidget(btnUser);
    root->addLayout(btnRow);

    root->addStretch();

    auto* hint = new QLabel("管理员: 出入库管理、查询、账户管理\n用户: 查询车位、预约车位", this);
    hint->setObjectName("captionLabel");
    hint->setAlignment(Qt::AlignCenter);
    root->addWidget(hint);
}

void RoleSelectDialog::onAdminClicked() {
    role_ = Admin;
    accept();
}

void RoleSelectDialog::onUserClicked() {
    role_ = User;
    accept();
}
