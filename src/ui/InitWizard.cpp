// ui/InitWizard.cpp
#include "InitWizard.h"
#include "core/DatabaseManager.h"
#include "core/Md5.h"
#include "core/Logger.h"
#include "services/AuthService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

InitWizard::InitWizard(QWidget* parent) : QDialog(parent) {
    setWindowTitle("停车场管理系统 - 首次初始化");
    setFixedSize(560, 640);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("欢迎使用停车场管理系统", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* subtitle = new QLabel("请填写停车场信息与初始管理员账户以完成初始化", this);
    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);

    // 停车场信息
    auto* gpPark = new QGroupBox("停车场信息", this);
    auto* parkForm = new QFormLayout(gpPark);
    parkNameEdit_  = new QLineEdit(this);
    parkNameEdit_->setPlaceholderText("如：新工停车场");
    feeSpin_       = new QDoubleSpinBox(this);
    feeSpin_->setRange(0.5, 999.0);
    feeSpin_->setValue(5.00);
    feeSpin_->setSuffix(" 元/小时");
    feeSpin_->setSingleStep(0.5);
    capacitySpin_  = new QSpinBox(this);
    capacitySpin_->setRange(1, 10000);
    capacitySpin_->setValue(100);
    parkForm->addRow("停车场名称:", parkNameEdit_);
    parkForm->addRow("停车单价:",   feeSpin_);
    parkForm->addRow("停车场容量:", capacitySpin_);
    root->addWidget(gpPark);

    // 管理员账户
    auto* gpAdmin = new QGroupBox("初始管理员账户", this);
    auto* adminForm = new QFormLayout(gpAdmin);
    adminUserEdit_  = new QLineEdit(this); adminUserEdit_->setPlaceholderText("≥3位字母数字下划线");
    adminUserEdit_->setText("admin");
    adminPwdEdit_   = new QLineEdit(this); adminPwdEdit_->setEchoMode(QLineEdit::Password);
    adminPwdEdit_->setPlaceholderText("≥6位");
    adminPwd2Edit_  = new QLineEdit(this); adminPwd2Edit_->setEchoMode(QLineEdit::Password);
    adminNameEdit_  = new QLineEdit(this); adminNameEdit_->setText("管理员");
    adminPhoneEdit_ = new QLineEdit(this); adminPhoneEdit_->setPlaceholderText("11位手机号");
    adminForm->addRow("账号:",     adminUserEdit_);
    adminForm->addRow("密码:",     adminPwdEdit_);
    adminForm->addRow("确认密码:", adminPwd2Edit_);
    adminForm->addRow("真实姓名:", adminNameEdit_);
    adminForm->addRow("手机号:",   adminPhoneEdit_);
    root->addWidget(gpAdmin);

    // HyperLPR3 模型路径
    auto* gpLpr = new QGroupBox("HyperLPR3 模型路径", this);
    auto* lprForm = new QFormLayout(gpLpr);
    auto* modelsRow = new QHBoxLayout;
    modelsPathEdit_ = new QLineEdit(this);
    modelsPathEdit_->setPlaceholderText("包含 *.mnn 模型文件的目录");

    // 默认模型路径查找顺序：
    //   1. 编译时宏 HYPERLPR3_DEFAULT_MODELS_PATH（CMake 自动注入）
    //   2. 工程内 third_party/hyperlpr3/resource/models/r2_mobile
    //   3. 用户本机 ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/resource/models/r2_mobile
    QString defaultModels;
#ifdef HYPERLPR3_DEFAULT_MODELS_PATH
    defaultModels = HYPERLPR3_DEFAULT_MODELS_PATH;
#endif
    if (defaultModels.isEmpty() || !QFileInfo::exists(defaultModels)) {
        // 回退 1：可执行文件同级 third_party
        QString appDir = QCoreApplication::applicationDirPath();
        QString candidates[] = {
            appDir + "/../third_party/hyperlpr3/resource/models/r2_mobile",
            appDir + "/third_party/hyperlpr3/resource/models/r2_mobile",
            QString("%1/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/resource/models/r2_mobile")
                .arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
        };
        for (const auto& c : candidates) {
            if (QFileInfo::exists(c)) { defaultModels = c; break; }
        }
        if (defaultModels.isEmpty()) defaultModels = candidates[2]; // 最后兜底
    }
    modelsPathEdit_->setText(defaultModels);
    auto* btnBrowse = new QPushButton("浏览...", this);
    connect(btnBrowse, &QPushButton::clicked, this, &InitWizard::onBrowseModels);
    modelsRow->addWidget(modelsPathEdit_);
    modelsRow->addWidget(btnBrowse);
    lprForm->addRow("模型目录:", modelsRow);
    root->addWidget(gpLpr);

    errLabel_ = new QLabel(this);
    errLabel_->setObjectName("errorLabel");
    errLabel_->setWordWrap(true);
    errLabel_->hide();
    root->addWidget(errLabel_);

    root->addStretch();

    // 按钮
    auto* btnRow = new QHBoxLayout;
    auto* btnCancel = new QPushButton("取消（退出）", this);
    auto* btnFinish = new QPushButton("完成初始化", this);
    btnFinish->setObjectName("primaryBtn");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnFinish, &QPushButton::clicked, this, &InitWizard::onFinish);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnFinish);
    root->addLayout(btnRow);
}

void InitWizard::onBrowseModels() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择 HyperLPR3 模型目录",
                                                     modelsPathEdit_->text());
    if (!dir.isEmpty()) modelsPathEdit_->setText(dir);
}

bool InitWizard::validate() {
    if (parkNameEdit_->text().trimmed().isEmpty()) { showError("请填写停车场名称"); return false; }
    if (parkNameEdit_->text().length() > 32)        { showError("停车场名称过长"); return false; }
    if (feeSpin_->value() <= 0)                     { showError("停车单价必须 > 0"); return false; }
    if (capacitySpin_->value() <= 0)                { showError("停车场容量必须 > 0"); return false; }

    QString u = adminUserEdit_->text().trimmed();
    QString p = adminPwdEdit_->text();
    QString p2 = adminPwd2Edit_->text();
    QString n = adminNameEdit_->text().trimmed();
    QString t = adminPhoneEdit_->text().trimmed();
    if (!AuthService::isValidUsername(u)) { showError("账号需≥3位且仅含字母数字下划线"); return false; }
    if (!AuthService::isValidPassword(p)) { showError("密码至少6位"); return false; }
    if (p != p2)                          { showError("两次密码不一致"); return false; }
    if (n.isEmpty())                      { showError("请填写管理员真实姓名"); return false; }
    if (!AuthService::isValidPhone(t))    { showError("手机号必须为11位且以1开头"); return false; }

    QString mp = modelsPathEdit_->text().trimmed();
    if (mp.isEmpty())                     { showError("请选择 HyperLPR3 模型目录"); return false; }
    QFileInfo fi(mp);
    if (!fi.isDir())                      { showError("模型目录不存在"); return false; }
    QDir d(mp);
    auto mnns = d.entryList(QStringList() << "*.mnn", QDir::Files);
    if (mnns.isEmpty())                   { showError("模型目录无 .mnn 文件"); return false; }
    return true;
}

void InitWizard::showError(const QString& msg) {
    errLabel_->setText(msg);
    errLabel_->show();
}

void InitWizard::onFinish() {
    if (!validate()) return;

    // 构造结果
    parking_.name     = parkNameEdit_->text().trimmed();
    parking_.allCount = capacitySpin_->value();
    parking_.fee      = feeSpin_->value();

    admin_.username  = adminUserEdit_->text().trimmed();
    admin_.password  = adminPwdEdit_->text();
    admin_.truename  = adminNameEdit_->text().trimmed();
    admin_.telephone = adminPhoneEdit_->text().trimmed();

    // 创建数据库 + schema + 种子数据
    QString dbPath = "data/parking.db";
    if (!DatabaseManager::instance().createWithSchema(dbPath, ":/sql/schema.sql")) {
        showError("数据库创建失败，请查看日志");
        return;
    }
    if (!DatabaseManager::instance().seedDefault(
            parking_.name, parking_.allCount, parking_.fee,
            admin_.username, Md5::hash(admin_.password),
            admin_.truename, admin_.telephone)) {
        showError("种子数据写入失败");
        return;
    }
    DatabaseManager::instance().setDbPath(dbPath);

    // 构造配置
    resultCfg_.dbPath              = dbPath;
    resultCfg_.parkingName         = parking_.name;
    resultCfg_.hyperlpr3ModelsPath = modelsPathEdit_->text().trimmed();
    resultCfg_.hyperlpr3LibPath    = QFileInfo(modelsPathEdit_->text().trimmed())
                                     .absolutePath() + "/../lib";
    resultCfg_.initialized         = true;
    resultCfg_.version             = 1;

    Logger::log(QtInfoMsg, "InitWizard: initialization completed");
    accept();
}
