// ui/ManualPlateDialog.cpp
#include "ManualPlateDialog.h"
#include "services/ReservationService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

ManualPlateDialog::ManualPlateDialog(const QString& hint, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("手动输入车牌");
    setFixedSize(360, 200);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(20, 20, 20, 20);

    auto* titleLabel = new QLabel("请输入车牌号", this);
    titleLabel->setObjectName("titleLabel");
    root->addWidget(titleLabel);

    if (!hint.isEmpty()) {
        auto* hintLabel = new QLabel(hint, this);
        hintLabel->setObjectName("captionLabel");
        hintLabel->setWordWrap(true);
        root->addWidget(hintLabel);
    }

    plateEdit_ = new QLineEdit(this);
    plateEdit_->setPlaceholderText("如：津B6H920 或 京AD12345");
    plateEdit_->setMaxLength(8);
    plateEdit_->setText(hint);
    plateEdit_->selectAll();
    root->addWidget(plateEdit_);

    errLabel_ = new QLabel(this);
    errLabel_->setObjectName("errorLabel");
    errLabel_->hide();
    root->addWidget(errLabel_);

    root->addStretch();

    auto* btnRow = new QHBoxLayout;
    auto* btnCancel = new QPushButton("取消", this);
    auto* btnOk = new QPushButton("确认", this);
    btnOk->setObjectName("primaryBtn");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &ManualPlateDialog::onConfirm);
    connect(plateEdit_, &QLineEdit::returnPressed, this, &ManualPlateDialog::onConfirm);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnOk);
    root->addLayout(btnRow);
}

void ManualPlateDialog::onConfirm() {
    QString p = plate();
    if (!ReservationService::isValidPlate(p)) {
        errLabel_->setText("车牌号不合法（7位普通或8位新能源）");
        errLabel_->show();
        plateEdit_->setStyleSheet("QLineEdit { border: 1px solid #F44336; }");
        return;
    }
    accept();
}
