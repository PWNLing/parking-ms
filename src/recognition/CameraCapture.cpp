// recognition/CameraCapture.cpp
#include "CameraCapture.h"
#include "core/Logger.h"
#include <QDir>
#include <QFileInfo>

CameraCapture::CameraCapture(QObject* parent) : QObject(parent) {
    pollTimer_.setInterval(33);  // ~30 fps
    connect(&pollTimer_, &QTimer::timeout, this, &CameraCapture::pollFrame);
}

CameraCapture::~CameraCapture() {
    stop();
}

QList<int> CameraCapture::listDevices() {
    QList<int> devs;
    QDir dir("/dev");
    QFileInfoList entries = dir.entryInfoList(QStringList() << "video*", QDir::System);
    for (const auto& fi : entries) {
        QString name = fi.fileName();  // video0, video1, ...
        QString num = name.mid(5);     // 去掉 "video" 前缀
        bool ok = false;
        int n = num.toInt(&ok);
        if (ok) devs.append(n);
    }
    std::sort(devs.begin(), devs.end());
    return devs;
}

bool CameraCapture::tryOpen(int index) {
    // 用 V4L2 后端打开 /dev/videoN
    cap_.open(index, cv::CAP_V4L2);
    if (!cap_.isOpened()) return false;

    // 设置 V4L2 超时（避免 select() 卡 10 秒）
    // 注意：OpenCV 的 V4L2 后端不直接支持超时设置，但可以通过设置缓冲区大小间接缓解
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // 尝试多种分辨率（从低到高，低分辨率更容易成功）
    struct Res { int w, h; };
    const Res resolutions[] = {{640,480}, {320,240}, {1280,720}};

    // 尝试不同格式：YUYV（默认，最兼容）→ MJPG（高清需此）
    // 不强制设 FOURCC，让驱动用默认值，避免不支持的格式导致 select 超时

    for (const auto& res : resolutions) {
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, res.w);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, res.h);

        // 试读一帧验证（grab + retrieve 分开，便于诊断）
        if (cap_.grab()) {
            cv::Mat test;
            if (cap_.retrieve(test) && !test.empty()) {
                Logger::log(QtInfoMsg, QString("  /dev/video%1: OK %2x%3")
                            .arg(index).arg(test.cols).arg(test.rows));
                return true;
            }
        }
        Logger::log(QtInfoMsg, QString("  /dev/video%1: %2x%3 grab failed")
                    .arg(index).arg(res.w).arg(res.h));
    }

    cap_.release();
    return false;
}

bool CameraCapture::start(int deviceIndex) {
    if (cap_.isOpened()) return true;  // 已打开

    // 列出可用设备
    QList<int> devs = listDevices();
    Logger::log(QtInfoMsg, QString("Available video devices: %1")
                .arg(devs.isEmpty() ? "(none)" : [&]{
                    QStringList s; for (int d : devs) s << QString("/dev/video%1").arg(d);
                    return s.join(", ");
                }()));

    if (devs.isEmpty()) {
        emit cameraError("未检测到 /dev/video* 设备");
        Logger::log(QtWarningMsg, "No /dev/video* devices found");
        return false;
    }

    // 指定设备号则只试该设备
    if (deviceIndex >= 0) {
        if (tryOpen(deviceIndex)) {
            deviceIndex_ = deviceIndex;
            Logger::log(QtInfoMsg, QString("Camera opened: /dev/video%1").arg(deviceIndex));
            polling_ = true;
            pollTimer_.start();
            return true;
        } else {
            Logger::log(QtWarningMsg, QString("Failed to open /dev/video%1").arg(deviceIndex));
            return false;
        }
    }

    // 自动检测：遍历所有设备，找到第一个能成功读帧的
    for (int idx : devs) {
        Logger::log(QtInfoMsg, QString("Trying /dev/video%1...").arg(idx));
        if (tryOpen(idx)) {
            deviceIndex_ = idx;
            Logger::log(QtInfoMsg, QString("Camera opened: /dev/video%1 (%2x%3)")
                        .arg(idx).arg(cap_.get(cv::CAP_PROP_FRAME_WIDTH))
                        .arg(cap_.get(cv::CAP_PROP_FRAME_HEIGHT)));
            polling_ = true;
            pollTimer_.start();
            return true;
        }
        Logger::log(QtInfoMsg, QString("/dev/video%1: no frames, skipping").arg(idx));
    }

    emit cameraError("所有 /dev/video* 设备均无法读取帧");
    Logger::log(QtWarningMsg, "No usable video device found");
    return false;
}

void CameraCapture::stop() {
    polling_ = false;
    pollTimer_.stop();
    if (cap_.isOpened()) {
        cap_.release();
        Logger::log(QtInfoMsg, "Camera released");
    }
    deviceIndex_ = -1;
}

void CameraCapture::pollFrame() {
    if (!cap_.isOpened()) return;

    cv::Mat bgr;
    if (!cap_.read(bgr) || bgr.empty()) {
        // 读帧失败（摄像头可能掉线）
        static int failCount = 0;
        failCount++;
        if (failCount <= 3) {
            Logger::log(QtWarningMsg, QString("Camera read failed (attempt %1)").arg(failCount));
        }
        if (failCount > 10) {
            Logger::log(QtCriticalMsg, "Camera read failed 10 times, stopping");
            polling_ = false;
            pollTimer_.stop();
            emit cameraError("摄像头读取失败（设备可能已掉线）");
        }
        return;
    }
    // 重置失败计数
    static int okCount = 0;
    okCount++;
    if (okCount == 1) {
        Logger::log(QtInfoMsg, QString("First frame: %1x%2").arg(bgr.cols).arg(bgr.rows));
    }

    // 转 QImage 发给 UI 显示
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
               QImage::Format_RGB888);
    QImage imgCopy = img.copy();  // 深拷贝，脱离 cv::Mat 生命周期
    emit frameImageReady(imgCopy);

    // 发 BGR Mat 给识别器
    emit frameReady(bgr.clone());
}