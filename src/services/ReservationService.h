// services/ReservationService.h — 预约查询/预约/清理过期
#pragma once
#include <QObject>
#include <QString>
#include "models/ParkingInfo.h"

class ReservationService : public QObject {
    Q_OBJECT
public:
    static ReservationService& instance();

    /// 查询停车场状态
    ParkingStatus queryStatus(const QString& parkingName);

    /// 预约车位；err 返回错误描述
    bool reserve(const QString& plate, const QString& parkingName, QString* err);

    /// 该车牌是否有有效预约
    bool hasReservation(const QString& plate);

    /// 清理过期预约（>30 分钟），返回清理条数
    int cleanExpired();

    /// 车牌合法性校验（与 VehicleService 共用规则）
    static bool isValidPlate(const QString& plate);

signals:
    /// 预约发生变化（新增/删除/清理）
    void reservationsChanged();
    /// 过期预约被清理，n 为条数
    void reservationsCleaned(int n);

private:
    ReservationService() = default;
};
