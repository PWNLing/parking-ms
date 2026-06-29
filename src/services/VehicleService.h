// services/VehicleService.h — 车辆出入库管理（核心业务）
#pragma once
#include <QObject>
#include <QString>
#include <QDateTime>
#include <optional>

struct EntryExitResult {
    bool    success      = false;
    QString message;            // Toast 文案
    QString action;             // "entry" / "exit"
    QString licensePlate;
    double  fee          = 0.0; // 仅出库有意义
    int     durationMin  = 0;
};

class VehicleService : public QObject {
    Q_OBJECT
public:
    static VehicleService& instance();

    /// 检测车牌当前可执行动作
    /// 返回 "entry"（可入库）/ "exit"（可出库）/ "none"
    QString detectAction(const QString& plate) const;

    /// 入库
    EntryExitResult entry(const QString& plate);

    /// 出库（自动按在场记录计费）
    EntryExitResult exit(const QString& plate);

    /// 入库时清除该车牌的有效预约（业务层操作，触发器自动 -1 P_reserve_count）
    bool clearReservationIfAny(const QString& plate);

    // ---- 静态工具 ----
    static bool   isValidPlate(const QString& plate);
    static int    calcDurationMin(const QDateTime& in, const QDateTime& out);
    static double calcFee(const QDateTime& in, const QDateTime& out, double unitPrice);

signals:
    /// 出入库成功后发射，UI 据此刷新饼图
    void parkingChanged();

private:
    VehicleService() = default;

    bool checkCapacity(const QString& parkingName);
    bool hasInProgressRecord(const QString& plate);
};
