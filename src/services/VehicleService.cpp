// services/VehicleService.cpp
#include "VehicleService.h"
#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "core/Logger.h"
#include "services/ReservationService.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QRegularExpression>
#include <cmath>

VehicleService& VehicleService::instance() {
    static VehicleService inst;
    return inst;
}

bool VehicleService::isValidPlate(const QString& plate) {
    return ReservationService::isValidPlate(plate); // 共用规则
}

int VehicleService::calcDurationMin(const QDateTime& in, const QDateTime& out) {
    if (!in.isValid() || !out.isValid()) return 0;
    return std::max(0, int(in.secsTo(out) / 60));
}

double VehicleService::calcFee(const QDateTime& in, const QDateTime& out, double unitPrice) {
    int mins = calcDurationMin(in, out);
    if (mins <= 30) return 0.0;                          // 半小时内免费
    int hours = static_cast<int>(std::ceil(mins / 60.0)); // 不足1小时按1小时
    return unitPrice * hours;
}

QString VehicleService::detectAction(const QString& plate) const {
    if (!isValidPlate(plate)) return "none";
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id FROM CAR WHERE license_plate = ? AND check_out_time IS NULL;");
    q.addBindValue(plate);
    q.exec();
    return q.next() ? "exit" : "entry";
}

bool VehicleService::checkCapacity(const QString& parkingName) {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT P_now_count, P_all_count FROM PARKING WHERE P_name = ?;");
    q.addBindValue(parkingName);
    if (q.exec() && q.next()) {
        return q.value(0).toInt() < q.value(1).toInt();
    }
    return false;
}

bool VehicleService::hasInProgressRecord(const QString& plate) {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id FROM CAR WHERE license_plate = ? AND check_out_time IS NULL;");
    q.addBindValue(plate);
    q.exec();
    return q.next();
}

bool VehicleService::clearReservationIfAny(const QString& plate) {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("DELETE FROM reservations WHERE license_plate = ?;");
    q.addBindValue(plate);
    return q.exec();
}

EntryExitResult VehicleService::entry(const QString& plate) {
    EntryExitResult r;
    r.action = "entry";
    r.licensePlate = plate;

    if (!isValidPlate(plate)) {
        r.message = "车牌号不合法";
        return r;
    }

    auto parkingName = ConfigManager::instance().current().parkingName;
    if (parkingName.isEmpty()) {
        r.message = "未配置停车场";
        return r;
    }

    if (hasInProgressRecord(plate)) {
        r.message = "该车辆已在场内，禁止重复入库";
        return r;
    }
    if (!checkCapacity(parkingName)) {
        r.message = "停车场已满，无法入库";
        return r;
    }

    // 获取当前单价
    auto db = DatabaseManager::instance().connection();
    QSqlQuery qFee(db);
    qFee.prepare("SELECT P_fee FROM PARKING WHERE P_name = ?;");
    qFee.addBindValue(parkingName);
    if (!qFee.exec() || !qFee.next()) {
        r.message = "停车场信息查询失败";
        return r;
    }
    double unitPrice = qFee.value(0).toDouble();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    bool ok = DatabaseManager::instance().transaction([&](QSqlDatabase& conn) -> bool {
        QSqlQuery q(conn);
        // 插入 CAR 记录
        q.prepare("INSERT INTO CAR(license_plate, check_in_time, location, unit_price) "
                  "VALUES (?, ?, ?, ?);");
        q.addBindValue(plate);
        q.addBindValue(now);
        q.addBindValue(parkingName);
        q.addBindValue(unitPrice);
        if (!q.exec()) return false;
        // 更新停车场在场数 +1
        q.prepare("UPDATE PARKING SET P_now_count = P_now_count + 1 WHERE P_name = ?;");
        q.addBindValue(parkingName);
        if (!q.exec()) return false;
        // 清除该车牌预约（若有，触发器自动 -1 P_reserve_count）
        q.prepare("DELETE FROM reservations WHERE license_plate = ?;");
        q.addBindValue(plate);
        if (!q.exec()) return false;
        return true;
    });

    if (!ok) {
        r.message = "入库失败（数据库事务回滚）";
        Logger::log(QtWarningMsg, QString("Entry txn failed: %1").arg(plate));
        return r;
    }

    r.success = true;
    r.message = QString("入库成功：%1 @ %2").arg(plate, now);
    Logger::log(QtInfoMsg, QString("Entry OK: %1 @ %2 fee=%.2f").arg(plate, now).arg(unitPrice));
    emit parkingChanged();
    return r;
}

EntryExitResult VehicleService::exit(const QString& plate) {
    EntryExitResult r;
    r.action = "exit";
    r.licensePlate = plate;

    if (!isValidPlate(plate)) {
        r.message = "车牌号不合法";
        return r;
    }

    auto db = DatabaseManager::instance().connection();
    QSqlQuery qFind(db);
    qFind.prepare("SELECT id, check_in_time, unit_price, location FROM CAR "
                  "WHERE license_plate = ? AND check_out_time IS NULL;");
    qFind.addBindValue(plate);
    if (!qFind.exec() || !qFind.next()) {
        r.message = "该车辆未在场内，无法出库";
        return r;
    }
    int recId      = qFind.value(0).toInt();
    QString inTime = qFind.value(1).toString();
    double unitP   = qFind.value(2).toDouble();
    QString loc    = qFind.value(3).toString();

    QDateTime inDt  = QDateTime::fromString(inTime, "yyyy-MM-dd HH:mm:ss");
    QDateTime outDt = QDateTime::currentDateTime();
    QString outTime = outDt.toString("yyyy-MM-dd HH:mm:ss");
    int mins = calcDurationMin(inDt, outDt);
    double fee = calcFee(inDt, outDt, unitP);

    bool ok = DatabaseManager::instance().transaction([&](QSqlDatabase& conn) -> bool {
        QSqlQuery q(conn);
        q.prepare("UPDATE CAR SET check_out_time = ?, fee = ? WHERE id = ?;");
        q.addBindValue(outTime);
        q.addBindValue(fee);
        q.addBindValue(recId);
        if (!q.exec()) return false;
        q.prepare("UPDATE PARKING SET P_now_count = P_now_count - 1 WHERE P_name = ?;");
        q.addBindValue(loc);
        if (!q.exec()) return false;
        return true;
    });

    if (!ok) {
        r.message = "出库失败（数据库事务回滚）";
        return r;
    }

    r.success = true;
    r.durationMin = mins;
    r.fee = fee;
    if (mins <= 30) {
        r.message = QString("出库成功：%1 | 停车%2分钟 | 免费").arg(plate).arg(mins);
    } else {
        r.message = QString("出库成功：%1 | 停车%2分钟 | 费用 ¥%3")
                    .arg(plate).arg(mins).arg(fee, 0, 'f', 2);
    }
    Logger::log(QtInfoMsg, QString("Exit OK: %1 mins=%2 fee=%.2f").arg(plate).arg(mins).arg(fee));
    emit parkingChanged();
    return r;
}
