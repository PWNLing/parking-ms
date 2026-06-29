// main.cpp — 应用入口
// 启动检测 → 初始化向导(首次) → 角色选择 → 登录(管理员) / 用户模式

#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include <QDir>
#include <QIcon>

#include "core/Logger.h"
#include "core/ConfigManager.h"
#include "core/DatabaseManager.h"
#include "core/Session.h"

#include "ui/InitWizard.h"
#include "ui/RoleSelectDialog.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"
#include "ui/UserModeWindow.h"

#include "recognition/PlateTypes.h"
#include "models/ParkingInfo.h"
#include "models/CarRecord.h"
#include "models/UserRow.h"
#include <opencv2/opencv.hpp>

/// 加载 QSS 主题
static void loadStyle(QApplication& app) {
    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    }
}

/// 首次启动检测 + 初始化
static bool ensureInitialized() {
    auto opt = ConfigManager::instance().load("config.json");
    if (opt && opt->initialized
        && DatabaseManager::instance().validate(opt->dbPath)) {
        DatabaseManager::instance().setDbPath(opt->dbPath);
        ConfigManager::instance().setCurrent(*opt);
        return true;
    }

    // 需要初始化
    Logger::log(QtInfoMsg, "First run or config invalid, launching InitWizard");
    InitWizard wizard;
    if (wizard.exec() != QDialog::Accepted) {
        Logger::log(QtInfoMsg, "InitWizard cancelled, exit");
        return false;
    }
    auto cfg = wizard.resultConfig();
    if (!ConfigManager::instance().save(cfg, "config.json")) {
        return false;
    }
    ConfigManager::instance().setCurrent(cfg);
    DatabaseManager::instance().setDbPath(cfg.dbPath);
    return true;
}

int main(int argc, char* argv[]) {
    // 高 DPI 支持（Qt6 默认开启）
    QApplication app(argc, argv);
    app.setApplicationName("ParkingMS");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ParkingMS");
    app.setWindowIcon(QIcon(":/icons/logo.svg"));

    // 使用 Fusion 风格让 QSS 表现更一致
    app.setStyle(QStyleFactory::create("Fusion"));

    // 注册元类型（跨线程信号槽必需）
    qRegisterMetaType<PlateResult>("PlateResult");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<ParkingStatus>("ParkingStatus");
    qRegisterMetaType<CarRecord>("CarRecord");
    qRegisterMetaType<UserRow>("UserRow");

    // 初始化日志
    Logger::init("logs");
    Logger::log(QtInfoMsg, "=== ParkingMS starting ===");

    // 加载 QSS
    loadStyle(app);

    // 初始化检测
    if (!ensureInitialized()) {
        return 0;
    }

    // 主循环：角色选择 → 各窗口（支持退出到角色选择的循环）
    int exitCode = 0;
    while (true) {
        // 清理会话
        SessionManager::instance().clear();

        RoleSelectDialog roleDlg;
        if (roleDlg.exec() != QDialog::Accepted) {
            Logger::log(QtInfoMsg, "Role selection cancelled, exit");
            break;
        }

        if (roleDlg.selectedRole() == RoleSelectDialog::Admin) {
            // 管理员：登录 → 主页
            LoginWindow login;
            if (login.exec() != QDialog::Accepted) {
                continue;  // 返回角色选择
            }
            MainWindow w;
            w.show();
            exitCode = app.exec();
        } else {
            // 用户：直接进用户模式
            UserModeWindow w;
            w.show();
            exitCode = app.exec();
        }

        // exitCode == 100 表示"切换角色"，继续循环
        if (exitCode == 100) {
            Logger::log(QtInfoMsg, "Switching role...");
            continue;
        }
        break;
    }

    Logger::log(QtInfoMsg, "=== ParkingMS exited ===");
    return exitCode;
}
