// ui/MainWindow.h — 管理员主页（饼图 + 摄像头 + 出入库）
#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <memory>
#include "recognition/PlateTypes.h"
#include "recognition/CameraCapture.h"
#include "recognition/PlateRecognizer.h"

#ifdef PARKING_MS_HAS_CHARTS
#include <QtCharts/QPieSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
// Qt6 中 QtCharts 类在全局命名空间，直接用未限定名（QPieSeries/QChart/QChartView/QPieSlice）
#endif

class PlateRecognizer;
class CameraCapture;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onPlateRecognized(const PlateResult& r);
    void onFrameRendered(const QImage& img);
    void onCameraError(const QString& reason);
    void onEntryClicked();
    void onExitClicked();
    void onSelectImageClicked();
    void refreshChart();
    void onSliceHover(bool hovered);
    void onAccountClicked();
    void onQueryClicked();
    void onChangePwdClicked();
    void onLogoutClicked();

private:
    void setupUi();
    void setupChart();
    void setupCamera();
    void setupTimer();
    void showToast(const QString& msg, bool success = true);
    void updateActionButtons(const QString& plate);

    // 摄像头与识别
    std::unique_ptr<PlateRecognizer> recognizer_;
    std::unique_ptr<CameraCapture>  camera_;
    bool cameraAvailable_ = false;

    // UI
    QLabel*           videoLabel_   = nullptr;
    QLabel*           resultLabel_  = nullptr;
    QLabel*           toastLabel_   = nullptr;
    QPushButton*      btnEntry_     = nullptr;
    QPushButton*      btnExit_      = nullptr;
    QPushButton*      btnSelectImg_ = nullptr;
    QLabel*           statusTextLabel_ = nullptr;  // 无 charts 时用文本显示车位
    QTimer*           refreshTimer_ = nullptr;
    QTimer*           toastTimer_   = nullptr;
    QString           pendingPlate_;

#ifdef PARKING_MS_HAS_CHARTS
    QPieSeries*       pieSeries_    = nullptr;
    QChartView*       chartView_    = nullptr;
#endif
};
