// ui/AccountDialog.cpp
#include "AccountDialog.h"
#include <QIcon>
#include "services/AccountService.h"
#include "core/Session.h"
#include "ui/ResetPwdDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

AccountDialog::AccountDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("账户管理");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    resize(720, 480);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(16, 16, 16, 16);

    // 操作按钮行
    auto* btnRow = new QHBoxLayout;
    auto* btnAdd     = new QPushButton("添加新用户", this);
    btnAdd->setObjectName("primaryBtn");
    auto* btnUpdate  = new QPushButton("修改选中", this);
    auto* btnDelete  = new QPushButton("删除选中", this);
    btnDelete->setObjectName("dangerBtn");
    auto* btnReset   = new QPushButton("重置密码", this);
    auto* btnReload  = new QPushButton("刷新", this);
    connect(btnAdd,    &QPushButton::clicked, this, &AccountDialog::onAdd);
    connect(btnUpdate, &QPushButton::clicked, this, &AccountDialog::onUpdate);
    connect(btnDelete, &QPushButton::clicked, this, &AccountDialog::onDelete);
    connect(btnReset,  &QPushButton::clicked, this, &AccountDialog::onResetPwd);
    connect(btnReload, &QPushButton::clicked, this, &AccountDialog::onReload);
    btnRow->addWidget(btnAdd);
    btnRow->addWidget(btnUpdate);
    btnRow->addWidget(btnDelete);
    btnRow->addWidget(btnReset);
    btnRow->addStretch();
    btnRow->addWidget(btnReload);
    root->addLayout(btnRow);

    // 表格
    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({"ID","账号","密码(MD5)","姓名","手机号"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    // 密码列只读
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setMaximumHeight(360);
    root->addWidget(table_, 1);

    hintLabel_ = new QLabel("提示：双击单元格可编辑姓名/手机号，修改后点击[修改选中]保存。\n"
                            "密码列密文显示，点击[重置密码]修改。", this);
    hintLabel_->setObjectName("captionLabel");
    hintLabel_->setWordWrap(true);
    root->addWidget(hintLabel_);

    auto* btnClose = new QPushButton("关闭", this);
    btnClose->setObjectName("primaryBtn");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    auto* closeRow = new QHBoxLayout;
    closeRow->addStretch();
    closeRow->addWidget(btnClose);
    root->addLayout(closeRow);

    loadUsers();
}

void AccountDialog::loadUsers() {
    auto users = AccountService().listAll();
    table_->setRowCount(static_cast<int>(users.size()));
    int row = 0;
    for (const auto& u : users) {
        auto* idItem = new QTableWidgetItem(QString::number(u.id));
        idItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        auto* userItem = new QTableWidgetItem(u.username);
        userItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        auto* pwdItem = new QTableWidgetItem(u.passwordHash);
        pwdItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable); // 只读
        auto* nameItem = new QTableWidgetItem(u.truename);
        auto* phoneItem = new QTableWidgetItem(u.telephone);
        table_->setItem(row, 0, idItem);
        table_->setItem(row, 1, userItem);
        table_->setItem(row, 2, pwdItem);
        table_->setItem(row, 3, nameItem);
        table_->setItem(row, 4, phoneItem);
        row++;
    }
}

int AccountDialog::currentUserId() {
    int row = table_->currentRow();
    if (row < 0) return -1;
    return table_->item(row, 0)->text().toInt();
}

void AccountDialog::onAdd() {
    // 在末尾空行输入
    int row = table_->rowCount();
    table_->insertRow(row);
    // 临时 ID
    auto* idItem = new QTableWidgetItem("(新)");
    idItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    auto* pwdItem = new QTableWidgetItem("(添加后自动 MD5)");
    pwdItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    table_->setItem(row, 0, idItem);
    table_->setItem(row, 1, new QTableWidgetItem(""));
    table_->setItem(row, 2, pwdItem);
    table_->setItem(row, 3, new QTableWidgetItem(""));
    table_->setItem(row, 4, new QTableWidgetItem(""));
    table_->editItem(table_->item(row, 1));
}

void AccountDialog::onUpdate() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一行");
        return;
    }
    QString idStr = table_->item(row, 0)->text();
    if (idStr == "(新)") {
        // 新增流程
        QString username = table_->item(row, 1)->text().trimmed();
        QString truename = table_->item(row, 3)->text().trimmed();
        QString telephone= table_->item(row, 4)->text().trimmed();
        // 密码通过 InputDialog 获取
        bool ok;
        QString pwd = QInputDialog::getText(this, "新用户密码", "请输入新用户密码:",
                                            QLineEdit::Password, "", &ok);
        if (!ok || pwd.isEmpty()) return;
        QString err;
        if (AccountService().addUser(username, pwd, truename, telephone, &err)) {
            loadUsers();
        } else {
            QMessageBox::warning(this, "添加失败", err);
        }
        return;
    }
    int id = idStr.toInt();
    QString truename = table_->item(row, 3)->text().trimmed();
    QString telephone= table_->item(row, 4)->text().trimmed();
    QString err;
    if (AccountService().updateUser(id, truename, telephone, &err)) {
        loadUsers();
    } else {
        QMessageBox::warning(this, "修改失败", err);
    }
}

void AccountDialog::onDelete() {
    int id = currentUserId();
    if (id < 0) { QMessageBox::warning(this, "提示", "请先选中一行"); return; }
    int cur = SessionManager::instance().isLoggedIn()
              ? SessionManager::instance().current()->userId : -1;
    auto ret = QMessageBox::question(this, "确认删除",
        QString("确认删除账户 ID=%1？").arg(id), QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    QString err;
    if (AccountService().deleteUser(id, cur, &err)) {
        loadUsers();
    } else {
        QMessageBox::warning(this, "删除失败", err);
    }
}

void AccountDialog::onResetPwd() {
    int id = currentUserId();
    if (id < 0) { QMessageBox::warning(this, "提示", "请先选中一行"); return; }
    ResetPwdDialog dlg(id, false, this);
    if (dlg.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "重置密码", "密码已重置");
    }
}

void AccountDialog::onReload() {
    loadUsers();
}
