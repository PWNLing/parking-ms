// ui/UserModeWindow.cpp
#include "UserModeWindow.h"
#include <QIcon>
#include "core/ConfigManager.h"
#include "core/Logger.h"
#include "services/ReservationService.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QStatusBar>

UserModeWindow::UserModeWindow(QWidget* parent)
    : QMainWindow(parent), settings_("ParkingMS", "User") {
    setupUi();
    setupTimer();

    // 监听预约变化
    connect(&ReservationService::instance(), &ReservationService::reservationsChanged,
            this, &UserModeWindow::refreshStatus);
    connect(&ReservationService::instance(), &ReservationService::reservationsCleaned,
            this, [this](int n){
        statusBar()->showMessage(QString("已清理 %1 个过期预约").arg(n), 5000);
        refreshStatus();
    });

    // 加载记忆的车牌
    QString lastPlate = settings_.value("lastPlate").toString();
    if (!lastPlate.isEmpty()) plateEdit_->setText(lastPlate);
}

UserModeWindow::~UserModeWindow() {
    if (refreshTimer_) refreshTimer_->stop();
    if (cleanTimer_)   cleanTimer_->stop();
}

void UserModeWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    refreshStatus();
}

void UserModeWindow::setupUi() {
    setWindowTitle("停车场管理系统 - 用户模式");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    resize(640, 560);
    setMinimumSize(560, 480);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setSpacing(16);
    root->setContentsMargins(24, 24, 24, 24);

    // 停车场名称
    nameLabel_ = new QLabel(ConfigManager::instance().current().parkingName, central);
    nameLabel_->setObjectName("titleLabel");
    nameLabel_->setAlignment(Qt::AlignCenter);
    root->addWidget(nameLabel_);

    // 状态卡片
    auto* gpStatus = new QGroupBox("车位状态", central);
    auto* statusLayout = new QVBoxLayout(gpStatus);
    statusLabel_ = new QLabel("加载中...", central);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("font-size: 16px; color: #1B5E20;");
    statusLayout->addWidget(statusLabel_);

    freeLabel_ = new QLabel("剩余: --", central);
    freeLabel_->setAlignment(Qt::AlignCenter);
    freeLabel_->setStyleSheet("font-size: 28px; font-weight: 700; color: #4CAF50;");
    statusLayout->addWidget(freeLabel_);

    reserveStatusLabel_ = new QLabel("您当前: 无预约", central);
    reserveStatusLabel_->setAlignment(Qt::AlignCenter);
    reserveStatusLabel_->setStyleSheet("font-size: 14px; color: #558B5E;");
    statusLayout->addWidget(reserveStatusLabel_);
    root->addWidget(gpStatus);

    // 预约区
    auto* gpReserve = new QGroupBox("预约车位", central);
    auto* reserveLayout = new QFormLayout(gpReserve);
    plateEdit_ = new QLineEdit(central);
    plateEdit_->setPlaceholderText("如：津B6H920");
    plateEdit_->setMaxLength(8);
    reserveLayout->addRow("我的车牌:", plateEdit_);

    btnReserve_ = new QPushButton("预约车位", central);
    btnReserve_->setObjectName("primaryBtn");
    connect(btnReserve_, &QPushButton::clicked, this, &UserModeWindow::onReserve);
    connect(plateEdit_, &QLineEdit::returnPressed, this, &UserModeWindow::onReserve);
    auto* reserveBtnRow = new QHBoxLayout;
    reserveBtnRow->addStretch();
    reserveBtnRow->addWidget(btnReserve_);
    reserveBtnRow->addStretch();
    reserveLayout->addRow(reserveBtnRow);
    root->addWidget(gpReserve);

    auto* hint = new QLabel("说明：预约后 30 分钟内入场有效，超时自动失效。\n"
                            "到场入场后预约自动消除。", central);
    hint->setObjectName("captionLabel");
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignCenter);
    root->addWidget(hint);

    root->addStretch();

    // 底部按钮
    auto* btnRow = new QHBoxLayout;
    auto* btnSwitch = new QPushButton("切换至管理员模式", central);
    auto* btnExit = new QPushButton("退出", central);
    connect(btnSwitch, &QPushButton::clicked, this, &UserModeWindow::onSwitchToAdmin);
    connect(btnExit, &QPushButton::clicked, this, &UserModeWindow::onExit);
    btnRow->addWidget(btnSwitch);
    btnRow->addStretch();
    btnRow->addWidget(btnExit);
    root->addLayout(btnRow);

    statusBar()->showMessage("用户模式");
}

void UserModeWindow::setupTimer() {
    // 状态刷新
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(2000);
    connect(refreshTimer_, &QTimer::timeout, this, &UserModeWindow::refreshStatus);
    refreshTimer_->start();

    // 过期预约清理（60 秒）
    cleanTimer_ = new QTimer(this);
    cleanTimer_->setInterval(60 * 1000);
    connect(cleanTimer_, &QTimer::timeout, [](){
        ReservationService::instance().cleanExpired();
    });
    cleanTimer_->start();
    // 启动时立即清理一次
    QMetaObject::invokeMethod(this, [this](){ ReservationService::instance().cleanExpired(); },
                              Qt::QueuedConnection);
}

void UserModeWindow::refreshStatus() {
    auto s = ReservationService::instance().queryStatus(
        ConfigManager::instance().current().parkingName);
    statusLabel_->setText(QString("总%1 | 入库%2 | 预约%3")
                          .arg(s.total).arg(s.inUse).arg(s.reserved));
    freeLabel_->setText(QString("剩余: %1").arg(s.free));
    if (s.isFull()) {
        freeLabel_->setStyleSheet("font-size: 28px; font-weight: 700; color: #F44336;");
        freeLabel_->setText("已满");
        btnReserve_->setEnabled(false);
    } else {
        freeLabel_->setStyleSheet("font-size: 28px; font-weight: 700; color: #4CAF50;");
        btnReserve_->setEnabled(true);
    }

    // 当前车牌预约状态
    QString plate = plateEdit_->text().trimmed().toUpper();
    if (!plate.isEmpty()) {
        bool has = ReservationService::instance().hasReservation(plate);
        reserveStatusLabel_->setText(has ? QString("您当前: 已预约 %1 ✓").arg(plate)
                                          : QString("您当前: 无预约"));
        reserveStatusLabel_->setStyleSheet(has
            ? "font-size: 14px; color: #4CAF50; font-weight: 600;"
            : "font-size: 14px; color: #558B5E;");
    }
}

void UserModeWindow::onReserve() {
    QString plate = plateEdit_->text().trimmed().toUpper();
    if (plate.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入车牌号");
        return;
    }
    QString err;
    if (ReservationService::instance().reserve(
            plate, ConfigManager::instance().current().parkingName, &err)) {
        // 记忆车牌
        settings_.setValue("lastPlate", plate);
        statusBar()->showMessage("预约成功！请于 30 分钟内入场", 5000);
        refreshStatus();
    } else {
        QMessageBox::warning(this, "预约失败", err);
    }
}

void UserModeWindow::onSwitchToAdmin() {
    if (refreshTimer_) refreshTimer_->stop();
    if (cleanTimer_)   cleanTimer_->stop();
    QApplication::exit(100); // 100 表示退出到角色选择
}

void UserModeWindow::onExit() {
    if (refreshTimer_) refreshTimer_->stop();
    if (cleanTimer_)   cleanTimer_->stop();
    QApplication::quit();
}
