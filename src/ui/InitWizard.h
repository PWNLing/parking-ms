// ui/InitWizard.h — 数据库初始化向导
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include "models/ParkingInfo.h"
#include "models/UserRow.h"
#include "core/ConfigManager.h"

class InitWizard : public QDialog {
    Q_OBJECT
public:
    explicit InitWizard(QWidget* parent = nullptr);

    AppConfig resultConfig() const { return resultCfg_; }
    ParkingInfo parkingInfo() const { return parking_; }
    AdminAccount adminAccount() const { return admin_; }

private slots:
    void onBrowseModels();
    void onFinish();

private:
    // 停车场
    QLineEdit*    parkNameEdit_ = nullptr;
    QDoubleSpinBox* feeSpin_    = nullptr;
    QSpinBox*     capacitySpin_ = nullptr;
    // 管理员
    QLineEdit*    adminUserEdit_  = nullptr;
    QLineEdit*    adminPwdEdit_   = nullptr;
    QLineEdit*    adminPwd2Edit_  = nullptr;
    QLineEdit*    adminNameEdit_  = nullptr;
    QLineEdit*    adminPhoneEdit_ = nullptr;
    // HyperLPR3
    QLineEdit*    modelsPathEdit_ = nullptr;
    // 错误
    QLabel*       errLabel_ = nullptr;

    AppConfig    resultCfg_;
    ParkingInfo  parking_;
    AdminAccount admin_;

    bool validate();
    void showError(const QString& msg);
};
