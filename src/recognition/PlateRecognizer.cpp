// recognition/PlateRecognizer.cpp
#include "PlateRecognizer.h"
#include "core/Logger.h"
#include <QImage>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QMetaObject>
#include <atomic>
#include <cstring>

PlateRecognizer::PlateRecognizer(const QString& modelsPath, QObject* parent)
    : QObject(nullptr), modelsPath_(modelsPath) {
    Q_UNUSED(parent);

    // 将本对象 move 到 worker 线程；之后所有槽函数都在 worker 执行
    this->moveToThread(&workerThread_);

    // worker 线程启动后立即初始化 Context（在 worker 线程内）
    connect(&workerThread_, &QThread::started, this, &PlateRecognizer::onWorkerInit);

    // worker 线程结束前清理 Context
    connect(&workerThread_, &QThread::finished, this, [this]{
        releaseContext();   // 在 worker 线程内释放
    });

    workerThread_.start();
}

PlateRecognizer::~PlateRecognizer() {
    // 1) 停止 worker 线程：触发 finished → releaseContext()（在 worker 线程内执行）
    workerThread_.quit();
    workerThread_.wait(5000);
    // 2) 此时 ctx_ 已在 worker 线程内释放完毕，对象可安全销毁
}

bool PlateRecognizer::isReady() const {
    return ready_.load(std::memory_order_acquire);
}

void PlateRecognizer::onWorkerInit() {
    // 在 worker 线程执行：创建 HyperLPR3 Context
    if (!ensureContextInWorker()) {
        lastError_ = "HyperLPR3 初始化失败，请检查模型路径: " + modelsPath_;
        Logger::log(QtCriticalMsg, lastError_);
        ready_ = false;
    } else {
        Logger::log(QtInfoMsg, QString("HyperLPR3 init OK: %1").arg(modelsPath_));
        ready_ = true;
    }
    emit readyChanged(ready_.load());
}

bool PlateRecognizer::ensureContextInWorker() {
    if (ctx_) return true;
    if (modelsPath_.isEmpty()) return false;
    QFileInfo fi(modelsPath_);
    if (!fi.isDir()) return false;

    QDir dir(modelsPath_);
    auto mnns = dir.entryList(QStringList() << "*.mnn", QDir::Files);
    if (mnns.isEmpty()) {
        lastError_ = "模型目录无 .mnn 文件: " + modelsPath_;
        return false;
    }

    // models_path 需在 Context 生命周期内保活
    static QByteArray modelsBytes;
    modelsBytes = modelsPath_.toUtf8();

    HLPR_ContextConfiguration cfg = {0};
    cfg.models_path              = const_cast<char*>(modelsBytes.constData());
    cfg.max_num                  = 5;
    cfg.det_level                = DETECT_LEVEL_LOW;
    cfg.use_half                 = false;
    cfg.nms_threshold            = 0.5f;
    cfg.box_conf_threshold       = 0.30f;
    cfg.rec_confidence_threshold = 0.5f;
    cfg.threads                  = 1;

    ctx_ = HLPR_CreateContext(&cfg);
    if (!ctx_) return false;
    return HLPR_ContextQueryStatus(ctx_) == HResultCode::Ok;
}

void PlateRecognizer::releaseContext() {
    if (ctx_) {
        HLPR_ReleaseContext(ctx_);
        ctx_ = nullptr;
        Logger::log(QtInfoMsg, "HyperLPR3 Context released");
    }
}

std::optional<PlateResult> PlateRecognizer::doRecognizeLocked(const cv::Mat& bgr) {
    if (!ctx_ || bgr.empty()) return std::nullopt;

    HLPR_ImageData data = {0};
    data.data     = const_cast<uint8_t*>(bgr.ptr<uint8_t>(0));
    data.width    = bgr.cols;
    data.height   = bgr.rows;
    data.format   = STREAM_BGR;
    data.rotation = CAMERA_ROTATION_0;

    P_HLPR_DataBuffer buffer = HLPR_CreateDataBuffer(&data);
    if (!buffer) return std::nullopt;

    HLPR_PlateResultList results = {0};
    HREESULT ret = HLPR_ContextUpdateStream(ctx_, buffer, &results);

    std::optional<PlateResult> out;
    if (ret == HResultCode::Ok && results.plate_size > 0) {
        int bestIdx = 0;
        float bestConf = -1;
        for (unsigned long i = 0; i < results.plate_size; ++i) {
            if (results.plates[i].text_confidence > bestConf) {
                bestConf = results.plates[i].text_confidence;
                bestIdx  = static_cast<int>(i);
            }
        }
        const HLPR_PlateResult& p = results.plates[bestIdx];
        if (p.text_confidence >= 0.5f) {
            PlateResult r;
            r.code       = QString::fromUtf8(p.code);
            r.type       = static_cast<int>(p.type);
            r.typeName   = plateTypeName(r.type);
            r.confidence = p.text_confidence;
            r.box        = QRectF(p.x1, p.y1, p.x2 - p.x1, p.y2 - p.y1);
            r.timestamp  = QDateTime::currentMSecsSinceEpoch();
            out = r;
        }
    }

    HLPR_ReleaseDataBuffer(buffer);  // 释放前已把数据拷出
    return out;
}

bool PlateRecognizer::shouldDebounce(const QString& code, qint64 now) {
    if (code == lastCode_ && (now - lastTs_) < 1000) return true;
    lastCode_ = code;
    lastTs_   = now;
    return false;
}

// void PlateRecognizer::recognizeMat(const cv::Mat& bgrImage) {
//     // 未就绪时丢弃帧（worker 线程还在初始化 Context）
//     if (!isReady()) return;
//     // 主线程调用：转发到 worker 线程执行 doRecognizeMat
//     // 用 QMetaObject::invokeMethod 保证跨线程安全（参数按值拷贝）
//     cv::Mat copy = bgrImage.clone();
//     QMetaObject::invokeMethod(this, "doRecognizeMat", Qt::QueuedConnection,
//                               Q_ARG(cv::Mat, copy));
// }

// void PlateRecognizer::doRecognizeMat(cv::Mat bgr) {
//     // 在 worker 线程执行
//     if (!ctx_) {
//         if (!ensureContextInWorker()) {
//             emit recognitionFailed("HyperLPR3 未初始化");
//             return;
//         }
//     }

//     cv::Mat bgrMat;
//     if (bgr.channels() == 4) {
//         cv::cvtColor(bgr, bgrMat, cv::COLOR_BGRA2BGR);
//     } else if (bgr.channels() == 3) {
//         bgrMat = bgr;
//     } else {
//         return;
//     }

//     auto opt = doRecognizeLocked(bgrMat);
//     if (!opt) return;

//     if (shouldDebounce(opt->code, opt->timestamp)) return;

//     cv::Mat annotated = drawAnnotation(bgrMat, *opt);
//     emit frameRendered(matToQImage(annotated));
//     emit plateRecognized(*opt);
// }

void PlateRecognizer::recognizeMat(const cv::Mat& bgrImage) {
    // 未就绪时丢弃帧（worker 线程还在初始化 Context）
    if (!isReady()) return;

    // 防止 worker 线程积压：如果上一帧还在识别（处理中标志为 true），丢弃新帧
    // HyperLPR3 识别一帧约 100-300ms，而摄像头 33ms 一帧，必须节流
    if (processing_.exchange(true)) {
        return;  // 已在处理中，丢弃此帧
    }

    // 主线程调用：转发到 worker 线程执行 doRecognizeMat
    cv::Mat copy = bgrImage.clone();
    QMetaObject::invokeMethod(this, "doRecognizeMat", Qt::QueuedConnection,
                              Q_ARG(cv::Mat, copy));
}

void PlateRecognizer::doRecognizeMat(cv::Mat bgr) {
    // 在 worker 线程执行
    // 保证退出时重置 processing_ 标志（RAII）
    struct ResetGuard {
        std::atomic<bool>& flag;
        ~ResetGuard() { flag.store(false); }
    } guard{processing_};

    if (!ctx_) {
        if (!ensureContextInWorker()) {
            emit recognitionFailed("HyperLPR3 未初始化");
            return;
        }
    }

    cv::Mat bgrMat;
    if (bgr.channels() == 4) {
        cv::cvtColor(bgr, bgrMat, cv::COLOR_BGRA2BGR);
    } else if (bgr.channels() == 3) {
        bgrMat = bgr;
    } else {
        return;
    }

    auto opt = doRecognizeLocked(bgrMat);

    // 诊断日志：每隔一定帧数输出识别状态（便于用户确认识别在工作）
    static std::atomic<int> frameCounter{0};
    int n = frameCounter.fetch_add(1) + 1;
    if (n % 30 == 1) {  // 每 30 帧输出一次
        if (opt) {
            Logger::log(QtInfoMsg, QString("Frame %1: recognized %2 (%3)")
                        .arg(n).arg(opt->code).arg(opt->confidence, 0, 'f', 2));
        } else {
            Logger::log(QtInfoMsg, QString("Frame %1: no plate detected").arg(n));
        }
    }

    if (!opt) return;

    if (shouldDebounce(opt->code, opt->timestamp)) {
        // 防抖：仍发射带框画面（让用户看到识别框），但不发射 plateRecognized
        cv::Mat annotated = drawAnnotation(bgrMat, *opt);
        emit frameRendered(matToQImage(annotated));
        return;
    }

    cv::Mat annotated = drawAnnotation(bgrMat, *opt);
    emit frameRendered(matToQImage(annotated));
    emit plateRecognized(*opt);
}

void PlateRecognizer::recognizeImageFile(const QString& path) {
    QMetaObject::invokeMethod(this, "doRecognizeImageFile", Qt::QueuedConnection,
                              Q_ARG(QString, path));
}

void PlateRecognizer::doRecognizeImageFile(QString path) {
    if (!ctx_) {
        if (!ensureContextInWorker()) {
            emit recognitionFailed("HyperLPR3 未初始化");
            return;
        }
    }
    cv::Mat img = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (img.empty()) {
        emit recognitionFailed("无法读取图片: " + path);
        return;
    }
    auto opt = doRecognizeLocked(img);
    if (!opt) {
        emit recognitionFailed("未识别到车牌");
        return;
    }
    cv::Mat annotated = drawAnnotation(img, *opt);
    emit frameRendered(matToQImage(annotated));
    emit plateRecognized(*opt);
}

QImage PlateRecognizer::matToQImage(const cv::Mat& mat) const {
    if (mat.empty()) return QImage();
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                  QImage::Format_RGB888).copy();  // copy 深拷贝，脱离 cv::Mat 生命周期
}

cv::Mat PlateRecognizer::drawAnnotation(const cv::Mat& src, const PlateResult& r) const {
    cv::Mat out = src.clone();
    cv::rectangle(out,
                  cv::Point2f(r.box.x(), r.box.y()),
                  cv::Point2f(r.box.x() + r.box.width(), r.box.y() + r.box.height()),
                  cv::Scalar(100, 200, 100), 2);
    std::string label = r.code.toStdString() + " " + r.typeName.toStdString();
    cv::putText(out, label, cv::Point2f(r.box.x(), r.box.y() - 8),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(100, 200, 100), 2);
    return out;
}