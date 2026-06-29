// recognition/CameraCapture.h — 摄像头取流（OpenCV V4L2 后端，Linux 最稳定）
#pragma once
#include <QObject>
#include <QTimer>
#include <QImage>
#include <opencv2/opencv.hpp>

/// 摄像头取流封装
///
/// 使用 OpenCV cv::VideoCapture (V4L2 后端) 直接读取 /dev/videoN
/// 相比 Qt6 Multimedia 的 FFmpeg 后端，V4L2 在 Linux USB 摄像头上更稳定可靠
///
/// 工作原理：
/// - QTimer 每 33ms (约 30fps) 调用 cap_.read() 取一帧
/// - 转 QImage 发 frameImageReady 信号（UI 显示）
/// - 转 BGR Mat 发 frameReady 信号（车牌识别）
class CameraCapture : public QObject {
    Q_OBJECT
public:
    explicit CameraCapture(QObject* parent = nullptr);
    ~CameraCapture() override;

    /// 启动摄像头；deviceIndex 指定 /dev/videoN 的 N（默认自动检测）
    bool start(int deviceIndex = -1);
    void stop();
    bool isAvailable() const { return cap_.isOpened(); }
    bool isActive() const { return cap_.isOpened() && polling_; }

    /// 列出可用的摄像头设备索引
    static QList<int> listDevices();

signals:
    /// 一帧 BGR cv::Mat 就绪（用于识别）
    void frameReady(const cv::Mat& bgr);
    /// 一帧 QImage 就绪（用于 UI 直接显示）
    void frameImageReady(const QImage& img);
    /// 摄像头错误
    void cameraError(const QString& reason);

private slots:
    void pollFrame();

private:
    cv::VideoCapture cap_;
    QTimer           pollTimer_;
    bool             polling_ = false;
    int              deviceIndex_ = -1;

    bool tryOpen(int index);
};