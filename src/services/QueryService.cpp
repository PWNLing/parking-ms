// services/QueryService.cpp
#include "QueryService.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"
#include <QSqlQuery>
#include <QSqlError>

static CarRecord recordFromQuery(QSqlQuery& q) {
    CarRecord r;
    r.id            = q.value(0).toInt();
    r.licensePlate  = q.value(1).toString();
    r.checkInTime   = q.value(2).toString();
    r.checkOutTime  = q.value(3).toString();   // 可能为 NULL → 空字符串
    r.fee           = q.value(4).isNull() ? -1.0 : q.value(4).toDouble();
    r.location      = q.value(5).toString();
    r.unitPrice     = q.value(6).toDouble();
    return r;
}

std::vector<CarRecord> QueryService::queryAll(int limit) {
    std::vector<CarRecord> out;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id, license_plate, check_in_time, check_out_time, fee, location, unit_price "
              "FROM CAR ORDER BY check_in_time DESC LIMIT ?;");
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(recordFromQuery(q));
    return out;
}

std::vector<CarRecord> QueryService::queryByPlate(const QString& plateLike, int limit) {
    std::vector<CarRecord> out;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id, license_plate, check_in_time, check_out_time, fee, location, unit_price "
              "FROM CAR WHERE license_plate LIKE ? || '%' "
              "ORDER BY check_in_time DESC LIMIT ?;");
    q.addBindValue(plateLike);
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(recordFromQuery(q));
    return out;
}

std::vector<CarRecord> QueryService::queryByTimeRange(const QDateTime& from, const QDateTime& to,
                                                       int limit) {
    std::vector<CarRecord> out;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id, license_plate, check_in_time, check_out_time, fee, location, unit_price "
              "FROM CAR WHERE check_in_time BETWEEN ? AND ? "
              "ORDER BY check_in_time DESC LIMIT ?;");
    q.addBindValue(from.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(to.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(recordFromQuery(q));
    return out;
}

std::vector<CarRecord> QueryService::queryByPlateAndTime(const QString& plateLike,
                                                          const QDateTime& from,
                                                          const QDateTime& to, int limit) {
    std::vector<CarRecord> out;
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.prepare("SELECT id, license_plate, check_in_time, check_out_time, fee, location, unit_price "
              "FROM CAR WHERE license_plate LIKE ? || '%' "
              "AND check_in_time BETWEEN ? AND ? "
              "ORDER BY check_in_time DESC LIMIT ?;");
    q.addBindValue(plateLike);
    q.addBindValue(from.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(to.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(recordFromQuery(q));
    return out;
}

bool QueryService::deleteRecord(int id, QString* err) {
    auto db = DatabaseManager::instance().connection();

    // 先查询该记录是否为"在场中"
    QSqlQuery qCheck(db);
    qCheck.prepare("SELECT check_out_time, location FROM CAR WHERE id = ?;");
    qCheck.addBindValue(id);
    if (!qCheck.exec() || !qCheck.next()) {
        if (err) *err = "记录不存在";
        return false;
    }
    bool inProgress = qCheck.value(0).isNull();
    QString loc = qCheck.value(1).toString();

    bool ok = DatabaseManager::instance().transaction([&](QSqlDatabase& conn) -> bool {
        QSqlQuery q(conn);
        q.prepare("DELETE FROM CAR WHERE id = ?;");
        q.addBindValue(id);
        if (!q.exec()) return false;
        if (inProgress) {
            q.prepare("UPDATE PARKING SET P_now_count = P_now_count - 1 WHERE P_name = ?;");
            q.addBindValue(loc);
            if (!q.exec()) return false;
        }
        return true;
    });

    if (!ok) {
        if (err) *err = "删除失败（事务回滚）";
        return false;
    }
    Logger::log(QtInfoMsg, QString("Car record deleted: id=%1 inProgress=%2").arg(id).arg(inProgress));
    return true;
}

int QueryService::totalCount() {
    auto db = DatabaseManager::instance().connection();
    QSqlQuery q(db);
    q.exec("SELECT COUNT(*) FROM CAR;");
    return q.next() ? q.value(0).toInt() : 0;
}

bool QueryService::updatePlate(int id, const QString& newPlate, QString* err) {
    // 车牌合法性校验
    QString plate = newPlate.trimmed().toUpper();
    if (plate.length() < 7 || plate.length() > 8) {
        if (err) *err = "车牌长度应为 7 或 8 位";
        return false;
    }
    static const QString provinces = "京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼使领";
    if (!provinces.contains(plate.left(1))) {
        if (err) *err = "车牌首字必须为省份简称";
        return false;
    }

    auto db = DatabaseManager::instance().connection();

    // 检查记录是否存在
    QSqlQuery qCheck(db);
    qCheck.prepare("SELECT id FROM CAR WHERE id = ?;");
    qCheck.addBindValue(id);
    if (!qCheck.exec() || !qCheck.next()) {
        if (err) *err = "记录不存在";
        return false;
    }

    // 检查新车牌是否与场内其他记录冲突
    QSqlQuery qDup(db);
    qDup.prepare("SELECT id FROM CAR WHERE license_plate = ? AND check_out_time IS NULL AND id != ?;");
    qDup.addBindValue(plate);
    qDup.addBindValue(id);
    if (qDup.exec() && qDup.next()) {
        if (err) *err = "该车牌已有一辆在场中的车，不能重复";
        return false;
    }

    QSqlQuery q(db);
    q.prepare("UPDATE CAR SET license_plate = ? WHERE id = ?;");
    q.addBindValue(plate);
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = QString("修改失败: %1").arg(q.lastError().text());
        return false;
    }
    Logger::log(QtInfoMsg, QString("Car plate updated: id=%1 → %2").arg(id).arg(plate));
    return true;
}
