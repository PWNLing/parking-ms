// recognition/PlateRecognizer.h — HyperLPR3 C API 封装 + QThread + 防抖
#pragma once
#include <QObject>
#include <QThread>
#include <QString>
#include <QImage>
#include <optional>
#include <atomic>
#include <opencv2/opencv.hpp>
#include "hyper_lpr_sdk.h"
#include "recognition/PlateTypes.h"

/// HyperLPR3 车牌识别器
///
/// 设计要点（避免崩溃）：
/// - HyperLPR3 Context 在 worker 线程内创建与销毁，保证 MNN 推理引擎的线程亲和性
/// - 主线程通过信号槽向 worker 提交识别请求；worker 通过信号把结果回主线程
/// - 析构顺序：先 quit worker → wait → 此时 worker 已释放 Context → 主线程再销毁对象
class PlateRecognizer : public QObject {
    Q_OBJECT
public:
    /// modelsPath 在构造时传入，worker 线程启动后据此初始化 Context
    explicit PlateRecognizer(const QString& modelsPath, QObject* parent = nullptr);
    ~PlateRecognizer() override;

    /// 是否就绪（Context 初始化完成）。线程安全。
    bool isReady() const;
    QString lastError() const { return lastError_; }

public slots:
    /// 识别 BGR cv::Mat（自动跨线程到 worker 执行）
    void recognizeMat(const cv::Mat& bgrImage);
    /// 识别本地图片文件
    void recognizeImageFile(const QString& path);

signals:
    void plateRecognized(const PlateResult& result);
    void recognitionFailed(const QString& reason);
    void frameRendered(const QImage& annotatedFrame);
    /// worker 初始化完成（成功或失败），主线程可据此更新 UI
    void readyChanged(bool ok);

private slots:
    /// 在 worker 线程执行：创建 Context
    void onWorkerInit();
    /// 在 worker 线程执行：识别一帧
    void doRecognizeMat(cv::Mat bgr);
    /// 在 worker 线程执行：识别图片
    void doRecognizeImageFile(QString path);

private:
    QString modelsPath_;
    QString lastError_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> processing_{false};  // 识别进行中标志（防止 worker 积压）

    // 仅在 worker 线程访问（无需锁）
    P_HLPR_Context ctx_ = nullptr;
    QString lastCode_;
    qint64  lastTs_ = 0;

    QThread workerThread_;

    void releaseContext();  // 在 worker 线程调用
    bool ensureContextInWorker();
    std::optional<PlateResult> doRecognizeLocked(const cv::Mat& bgr);
    bool shouldDebounce(const QString& code, qint64 now);
    QImage matToQImage(const cv::Mat& mat) const;
    cv::Mat drawAnnotation(const cv::Mat& src, const PlateResult& r) const;
};
