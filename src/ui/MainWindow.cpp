// ui/MainWindow.cpp
#include "MainWindow.h"
#include "core/ConfigManager.h"
#include "core/Session.h"
#include "core/Logger.h"
#include "services/VehicleService.h"
#include "services/ReservationService.h"
#include "ui/ManualPlateDialog.h"
#include "ui/QueryDialog.h"
#include "ui/AccountDialog.h"
#include "ui/ResetPwdDialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QBrush>
#include <QColor>
#include <QStatusBar>
#include <QCloseEvent>
#include <QDir>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    setupChart();
    setupCamera();
    setupTimer();

    connect(&VehicleService::instance(), &VehicleService::parkingChanged,
            this, &MainWindow::refreshChart);
    connect(&ReservationService::instance(), &ReservationService::reservationsChanged,
            this, &MainWindow::refreshChart);
}

MainWindow::~MainWindow() {
    // 析构顺序很关键，避免崩溃：
    // 1. 停止定时器（不再触发 refreshChart）
    if (refreshTimer_) refreshTimer_->stop();
    // 2. 停止摄像头（不再产生新帧）
    if (camera_) camera_->stop();
    // 3. 断开 recognizer 信号（避免 worker 线程还在发信号到已销毁的 this）
    if (recognizer_) recognizer_->disconnect();
    // 4. 显式 reset recognizer（触发 worker quit+wait，在 QApplication 退出前完成）
    recognizer_.reset();
    camera_.reset();
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    refreshChart();
    // 仅在摄像头可用且未运行时启动（避免重复 start 导致 "Camera is in use"）
    if (cameraAvailable_ && camera_ && !camera_->isActive()) {
        cameraAvailable_ = camera_->start();
        if (!cameraAvailable_) {
            videoLabel_->setText("摄像头启动失败（可能被占用）\n请点击 [选择本地图片识别]");
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (camera_) camera_->stop();
    QMainWindow::closeEvent(e);
}

void MainWindow::setupUi() {
    setWindowTitle("停车场管理系统 - 管理员主页");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    resize(1280, 800);
    setMinimumSize(1024, 720);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setSpacing(12);
    root->setContentsMargins(16, 16, 16, 16);

    // 顶部标题栏
    auto* topRow = new QHBoxLayout;
    auto* titleLabel = new QLabel(ConfigManager::instance().current().parkingName, central);
    titleLabel->setObjectName("titleLabel");
    topRow->addWidget(titleLabel);

    auto status = ReservationService::instance().queryStatus(
        ConfigManager::instance().current().parkingName);
    auto* feeLabel = new QLabel(QString("单价: ¥%1 / 小时").arg(status.fee, 0, 'f', 2), central);
    feeLabel->setObjectName("subtitleLabel");
    topRow->addWidget(feeLabel);

    topRow->addStretch();

    auto* timeLabel = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"), central);
    timeLabel->setObjectName("subtitleLabel");
    topRow->addWidget(timeLabel);
    auto* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, [timeLabel](){
        timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    });
    clockTimer->start(30000);
    root->addLayout(topRow);

    // 中部：摄像头 + 饼图
    auto* midRow = new QHBoxLayout;
    midRow->setSpacing(12);

    // 左：摄像头画面
    auto* gpVideo = new QGroupBox("摄像头画面", central);
    auto* videoLayout = new QVBoxLayout(gpVideo);
    videoLabel_ = new QLabel(gpVideo);
    videoLabel_->setMinimumSize(640, 480);
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setStyleSheet("background: #2E3B2E; color: #A5D6A7; font-size: 16px;");
    videoLabel_->setText("摄像头启动中...");
    videoLayout->addWidget(videoLabel_);

    auto* videoBtnRow = new QHBoxLayout;
    btnSelectImg_ = new QPushButton("选择本地图片识别", gpVideo);
    connect(btnSelectImg_, &QPushButton::clicked, this, &MainWindow::onSelectImageClicked);
    videoBtnRow->addStretch();
    videoBtnRow->addWidget(btnSelectImg_);
    videoLayout->addLayout(videoBtnRow);
    midRow->addWidget(gpVideo, 3);

    // 右：饼图 或 文本状态
    auto* gpChart = new QGroupBox("车位使用情况", central);
    auto* chartLayout = new QVBoxLayout(gpChart);
#ifdef PARKING_MS_HAS_CHARTS
    chartView_ = new QChartView(gpChart);
    chartView_->setRenderHint(QPainter::Antialiasing);
    chartView_->setMinimumSize(360, 480);
    chartLayout->addWidget(chartView_);
#else
    statusTextLabel_ = new QLabel(gpChart);
    statusTextLabel_->setAlignment(Qt::AlignCenter);
    statusTextLabel_->setStyleSheet("font-size: 18px; color: #1B5E20; padding: 20px;");
    statusTextLabel_->setMinimumSize(360, 480);
    chartLayout->addWidget(statusTextLabel_);
#endif
    midRow->addWidget(gpChart, 2);
    root->addLayout(midRow, 5);

    // 识别结果栏
    auto* gpResult = new QGroupBox("识别结果", central);
    auto* resultLayout = new QHBoxLayout(gpResult);
    resultLabel_ = new QLabel("等待识别...", gpResult);
    resultLabel_->setStyleSheet("font-size: 16px; font-weight: 600; color: #1B5E20;");
    resultLayout->addWidget(resultLabel_, 1);

    btnEntry_ = new QPushButton("确认入库", gpResult);
    btnEntry_->setObjectName("primaryBtn");
    btnEntry_->setEnabled(false);
    connect(btnEntry_, &QPushButton::clicked, this, &MainWindow::onEntryClicked);

    btnExit_ = new QPushButton("确认出库", gpResult);
    btnExit_->setObjectName("dangerBtn");
    btnExit_->setEnabled(false);
    connect(btnExit_, &QPushButton::clicked, this, &MainWindow::onExitClicked);

    resultLayout->addWidget(btnEntry_);
    resultLayout->addWidget(btnExit_);
    root->addWidget(gpResult);

    // 功能导航
    auto* navRow = new QHBoxLayout;
    auto* btnQuery     = new QPushButton("车库信息", central);
    auto* btnAccount   = new QPushButton("账户管理", central);
    auto* btnChangePwd = new QPushButton("修改密码", central);
    auto* btnLogout    = new QPushButton("退出登录", central);
    connect(btnQuery,     &QPushButton::clicked, this, &MainWindow::onQueryClicked);
    connect(btnAccount,   &QPushButton::clicked, this, &MainWindow::onAccountClicked);
    connect(btnChangePwd, &QPushButton::clicked, this, &MainWindow::onChangePwdClicked);
    connect(btnLogout,    &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    navRow->addWidget(btnQuery);
    navRow->addWidget(btnAccount);
    navRow->addWidget(btnChangePwd);
    navRow->addStretch();
    navRow->addWidget(btnLogout);
    root->addLayout(navRow);

    toastLabel_ = new QLabel(central);
    toastLabel_->setObjectName("toastLabel");
    toastLabel_->setAlignment(Qt::AlignCenter);
    toastLabel_->hide();
    root->addWidget(toastLabel_);

    statusBar()->showMessage("就绪");
}

void MainWindow::setupChart() {
#ifdef PARKING_MS_HAS_CHARTS
    auto* chart = new QChart;
    pieSeries_ = new QPieSeries;
    pieSeries_->setHoleSize(0.3);
    chart->addSeries(pieSeries_);
    chart->setTitle(ConfigManager::instance().current().parkingName);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundBrush(QBrush(QColor(255, 255, 255)));
    chartView_->setChart(chart);
#endif
}

void MainWindow::setupCamera() {
    auto modelsPath = ConfigManager::instance().current().hyperlpr3ModelsPath;
    recognizer_ = std::make_unique<PlateRecognizer>(modelsPath, nullptr);  // 无 parent，便于 moveToThread

    // 注意：构造时 isReady() 可能返回 false（worker 线程异步初始化中），
    // 不在此弹框，改用 readyChanged 信号异步通知（避免竞态误弹 QMessageBox）
    connect(recognizer_.get(), &PlateRecognizer::readyChanged,
            this, [this](bool ok){
        if (!ok) {
            QMessageBox::warning(this, "HyperLPR3", recognizer_->lastError());
        }
    }, Qt::QueuedConnection);
    connect(recognizer_.get(), &PlateRecognizer::plateRecognized,
            this, &MainWindow::onPlateRecognized, Qt::QueuedConnection);
    connect(recognizer_.get(), &PlateRecognizer::frameRendered,
            this, &MainWindow::onFrameRendered, Qt::QueuedConnection);
    connect(recognizer_.get(), &PlateRecognizer::recognitionFailed,
            this, [this](const QString& r){ showToast(r, false); });

    camera_ = std::make_unique<CameraCapture>(this);
    connect(camera_.get(), &CameraCapture::frameReady,
            recognizer_.get(), &PlateRecognizer::recognizeMat, Qt::QueuedConnection);
    // 关键：连接原始帧信号，让画面持续刷新（无论是否识别到车牌）
    connect(camera_.get(), &CameraCapture::frameImageReady,
            this, [this](const QImage& img){
        if (!img.isNull()) {
            videoLabel_->setPixmap(QPixmap::fromImage(img).scaled(
                videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }, Qt::QueuedConnection);
    connect(camera_.get(), &CameraCapture::cameraError,
            this, &MainWindow::onCameraError);

    cameraAvailable_ = camera_->start();
    if (!cameraAvailable_) {
        videoLabel_->setText("未检测到摄像头（或未启用 Multimedia 模块）\n请点击 [选择本地图片识别]");
    }
}

void MainWindow::setupTimer() {
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(2000);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::refreshChart);
    refreshTimer_->start();

    toastTimer_ = new QTimer(this);
    toastTimer_->setSingleShot(true);
    connect(toastTimer_, &QTimer::timeout, [this](){ toastLabel_->hide(); });
}

void MainWindow::refreshChart() {
    auto status = ReservationService::instance().queryStatus(
        ConfigManager::instance().current().parkingName);

#ifdef PARKING_MS_HAS_CHARTS
    pieSeries_->clear();
    auto* sReserve = pieSeries_->append("预约数", status.reserved);
    auto* sIn      = pieSeries_->append("入库数", status.inUse);
    sIn->setColor(QColor("#FFA500"));
    auto* sFree    = pieSeries_->append("剩余数", status.free);

    for (auto* slice : pieSeries_->slices()) {
        slice->setLabel(slice->label() + QString::asprintf(":%.0f", slice->value()));
        slice->setLabelVisible(slice->value() >= 10);
        connect(slice, &QPieSlice::hovered, this, &MainWindow::onSliceHover);
    }
    sFree->setExploded(true);
    sFree->setExplodeDistanceFactor(0.1);
    (void)sReserve;
#else
    if (statusTextLabel_) {
        statusTextLabel_->setText(QString("总车位: %1\n\n入库: %2\n预约: %3\n\n剩余: %4")
                                  .arg(status.total).arg(status.inUse)
                                  .arg(status.reserved).arg(status.free));
    }
#endif

    if (status.isFull()) {
        statusBar()->showMessage("⚠ 停车场已满！", 3000);
    } else {
        statusBar()->showMessage(QString("总%1 | 入库%2 | 预约%3 | 剩余%4")
                                 .arg(status.total).arg(status.inUse)
                                 .arg(status.reserved).arg(status.free));
    }
}

void MainWindow::onSliceHover(bool hovered) {
#ifdef PARKING_MS_HAS_CHARTS
    auto* slice = qobject_cast<QPieSlice*>(sender());
    if (slice) slice->setExploded(hovered);
#else
    Q_UNUSED(hovered);
#endif
}

void MainWindow::onPlateRecognized(const PlateResult& r) {
    QString text = QString("%1 | %2 | 置信度 %3")
                   .arg(r.code).arg(r.typeName).arg(r.confidence, 0, 'f', 2);
    resultLabel_->setText(text);
    updateActionButtons(r.code);
}

void MainWindow::onFrameRendered(const QImage& img) {
    if (!img.isNull()) {
        videoLabel_->setPixmap(QPixmap::fromImage(img).scaled(
            videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::onCameraError(const QString& reason) {
    videoLabel_->setText("摄像头错误: " + reason + "\n请改用 [选择本地图片识别]");
    cameraAvailable_ = false;
}

void MainWindow::onSelectImageClicked() {
    QString path = QFileDialog::getOpenFileName(this, "选择车牌图片",
        QDir::homePath(), "图片 (*.jpg *.jpeg *.png *.bmp)");
    if (path.isEmpty()) return;
    if (recognizer_ && recognizer_->isReady()) {
        QMetaObject::invokeMethod(recognizer_.get(), "recognizeImageFile",
                                  Qt::QueuedConnection, Q_ARG(QString, path));
    } else {
        QMessageBox::warning(this, "HyperLPR3", "识别器未就绪");
    }
}

void MainWindow::updateActionButtons(const QString& plate) {
    pendingPlate_ = plate;
    QString action = VehicleService::instance().detectAction(plate);
    if (action == "entry") {
        btnEntry_->setEnabled(true);
        btnExit_->setEnabled(false);
        btnEntry_->setFocus();
    } else if (action == "exit") {
        btnEntry_->setEnabled(false);
        btnExit_->setEnabled(true);
        btnExit_->setFocus();
    } else {
        btnEntry_->setEnabled(false);
        btnExit_->setEnabled(false);
    }
}

void MainWindow::onEntryClicked() {
    QString plate = pendingPlate_;
    if (plate.isEmpty() || !VehicleService::isValidPlate(plate)) {
        ManualPlateDialog dlg("识别车牌不合法，请手动输入", this);
        if (dlg.exec() != QDialog::Accepted) return;
        plate = dlg.plate();
    }
    auto r = VehicleService::instance().entry(plate);
    showToast(r.message, r.success);
    if (r.success) {
        pendingPlate_.clear();
        btnEntry_->setEnabled(false);
        btnExit_->setEnabled(false);
        resultLabel_->setText("等待识别...");
    }
}

void MainWindow::onExitClicked() {
    QString plate = pendingPlate_;
    if (plate.isEmpty() || !VehicleService::isValidPlate(plate)) {
        ManualPlateDialog dlg("识别车牌不合法，请手动输入", this);
        if (dlg.exec() != QDialog::Accepted) return;
        plate = dlg.plate();
    }
    auto r = VehicleService::instance().exit(plate);
    showToast(r.message, r.success);
    if (r.success) {
        pendingPlate_.clear();
        btnEntry_->setEnabled(false);
        btnExit_->setEnabled(false);
        resultLabel_->setText("等待识别...");
    }
}

void MainWindow::showToast(const QString& msg, bool success) {
    toastLabel_->setObjectName(success ? "toastSuccess" : "toastError");
    toastLabel_->setText(msg);
    toastLabel_->show();
    toastTimer_->start(3000);
}

void MainWindow::onQueryClicked() {
    QueryDialog dlg(this);
    dlg.exec();
    refreshChart();
}

void MainWindow::onAccountClicked() {
    AccountDialog dlg(this);
    dlg.exec();
}

void MainWindow::onChangePwdClicked() {
    auto& sess = SessionManager::instance();
    if (!sess.isLoggedIn()) return;
    ResetPwdDialog dlg(sess.current()->userId, true, this);
    if (dlg.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "修改密码", "密码修改成功，请重新登录");
        onLogoutClicked();
    }
}

void MainWindow::onLogoutClicked() {
    SessionManager::instance().clear();
    if (camera_) camera_->stop();
    if (refreshTimer_) refreshTimer_->stop();
    QApplication::exit(100);
}
