// ui/ManualPlateDialog.h — 手动输入车牌（识别失败/不合法时回退）
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class ManualPlateDialog : public QDialog {
    Q_OBJECT
public:
    explicit ManualPlateDialog(const QString& hint, QWidget* parent = nullptr);
    QString plate() const { return plateEdit_->text().trimmed().toUpper(); }

private slots:
    void onConfirm();

private:
    QLineEdit* plateEdit_ = nullptr;
    QLabel*    errLabel_  = nullptr;
};
