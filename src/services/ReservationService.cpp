// services/ReservationService.cpp
#include "ReservationService.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>

ReservationService& ReservationService::instance() {
    static ReservationService inst;
    return inst;
}

bool ReservationService::isValidPlate(const QString& plate) {
    // 兼容普通 7 位 + 新能源 8 位
    static QRegularExpression re(
        "^[京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼使领]"
        "[A-HJ-NP-Z]"
        "[A-HJ-NP-Z0-9]{4,5}"
        "[A-HJ-NP-Z0-9]$");
    return re.match(plate).hasMatch() && (plate.length() == 7 || plate.length() == 8);
}

ParkingStatus ReservationService::queryStatus(const QString& parkingName) {
    ParkingStatus s;
    s.name = parkingName;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT P_name, P_now_count, P_reserve_count, P_all_count, P_fee "
              "FROM PARKING WHERE P_name = ?;");
    q.addBindValue(parkingName);
    if (q.exec() && q.next()) {
        s.name     = q.value(0).toString();
        s.inUse    = q.value(1).toInt();
        s.reserved = q.value(2).toInt();
        s.total    = q.value(3).toInt();
        s.fee      = q.value(4).toDouble();
        s.free     = s.total - s.inUse - s.reserved;
    }
    return s;
}

bool ReservationService::reserve(const QString& plate, const QString& parkingName, QString* err) {
    if (!isValidPlate(plate)) {
        if (err) *err = "车牌号不合法（应为7位普通或8位新能源车牌）";
        return false;
    }
    auto status = queryStatus(parkingName);
    if (status.free <= 0) {
        if (err) *err = "车位已满，暂无法预约";
        return false;
    }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("INSERT INTO reservations(license_plate, P_name) VALUES (?, ?);");
    q.addBindValue(plate);
    q.addBindValue(parkingName);
    if (!q.exec()) {
        // UNIQUE 约束违反
        QString e = q.lastError().text();
        if (e.contains("UNIQUE", Qt::CaseInsensitive)) {
            if (err) *err = "该车牌已有有效预约，无需重复预约";
        } else {
            if (err) *err = QString("预约失败: %1").arg(e);
        }
        return false;
    }
    Logger::log(QtInfoMsg, QString("Reservation OK: %1 @ %2").arg(plate, parkingName));
    emit reservationsChanged();
    return true;
}

bool ReservationService::hasReservation(const QString& plate) {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id FROM reservations WHERE license_plate = ?;");
    q.addBindValue(plate);
    return q.exec() && q.next();
}

int ReservationService::cleanExpired() {
    auto db = DatabaseManager::instance().connection();
    // 先查询条数（便于通知 UI）
    QSqlQuery q1(db);
    q1.exec("SELECT COUNT(*) FROM reservations "
            "WHERE created_at < datetime('now','localtime','-30 minutes');");
    int n = (q1.next() ? q1.value(0).toInt() : 0);
    if (n == 0) return 0;

    QSqlQuery q2(db);
    if (!q2.exec("DELETE FROM reservations "
                 "WHERE created_at < datetime('now','localtime','-30 minutes');")) {
        Logger::log(QtWarningMsg, QString("Clean expired failed: %1").arg(q2.lastError().text()));
        return 0;
    }
    Logger::log(QtInfoMsg, QString("Cleaned %1 expired reservations").arg(n));
    emit reservationsCleaned(n);
    emit reservationsChanged();
    return n;
}
