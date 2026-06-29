// services/QueryService.h — 车库信息查询
#pragma once
#include <QString>
#include <QDateTime>
#include <vector>
#include "models/CarRecord.h"

class QueryService {
public:
    /// 全部记录（按入库时间倒序）
    std::vector<CarRecord> queryAll(int limit = 1000);

    /// 按车牌模糊查询
    std::vector<CarRecord> queryByPlate(const QString& plateLike, int limit = 1000);

    /// 按时间段查询
    std::vector<CarRecord> queryByTimeRange(const QDateTime& from, const QDateTime& to,
                                             int limit = 1000);

    /// 组合查询（车牌 + 时间段）
    std::vector<CarRecord> queryByPlateAndTime(const QString& plateLike,
                                                const QDateTime& from, const QDateTime& to,
                                                int limit = 1000);

    /// 删除一条记录；若为"在场中"记录，同步 P_now_count -1
    bool deleteRecord(int id, QString* err);

    /// 修改一条记录的车牌号（用于识别不准时人工修正）
    /// 传记录 id 与新车牌号；若车牌不合法返回 false
    bool updatePlate(int id, const QString& newPlate, QString* err);

    /// 总条数
    int totalCount();
};
