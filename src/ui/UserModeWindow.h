// ui/UserModeWindow.h — 用户模式主页（车位查询 + 预约）
#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QSettings>

class UserModeWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit UserModeWindow(QWidget* parent = nullptr);
    ~UserModeWindow() override;

protected:
    void showEvent(QShowEvent* e) override;

private slots:
    void onReserve();
    void refreshStatus();
    void onSwitchToAdmin();
    void onExit();

private:
    void setupUi();
    void setupTimer();

    QLabel*     nameLabel_     = nullptr;
    QLabel*     statusLabel_   = nullptr;
    QLabel*     freeLabel_     = nullptr;
    QLabel*     reserveStatusLabel_ = nullptr;
    QLineEdit*  plateEdit_     = nullptr;
    QPushButton* btnReserve_   = nullptr;
    QTimer*     refreshTimer_  = nullptr;
    QTimer*     cleanTimer_    = nullptr;
    QSettings   settings_;
};
