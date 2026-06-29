// models/CarRecord.h — 车辆出入库记录
#pragma once
#include <QString>
#include <QMetaType>
#include <optional>

struct CarRecord {
    int     id = 0;
    QString licensePlate;
    QString checkInTime;    // ISO8601 'YYYY-MM-DD HH:MM:SS'
    QString checkOutTime;   // 空 = 在场中
    double  fee = -1.0;     // -1 = 未结算，0 = 免费，>0 = 已计费
    QString location;
    double  unitPrice = 0.0;

    bool isInProgress() const { return checkOutTime.isEmpty(); }
    QString feeDisplay() const {
        if (fee < 0) return "—";
        if (fee == 0) return "免费";
        return QString::number(fee, 'f', 2);
    }
    QString checkOutDisplay() const {
        return checkOutTime.isEmpty() ? "—" : checkOutTime;
    }
};

Q_DECLARE_METATYPE(CarRecord)
