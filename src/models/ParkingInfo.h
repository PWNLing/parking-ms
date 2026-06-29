// models/ParkingInfo.h — 停车场信息与状态结构
#pragma once
#include <QString>
#include <QMetaType>

/// 停车场初始化信息（向导录入）
struct ParkingInfo {
    QString name;          // 停车场名称
    int     allCount = 0;  // 总容量
    double  fee = 0.0;     // 单价 元/小时
};

/// 停车场运行时状态（饼图展示用）
struct ParkingStatus {
    QString name;
    int total    = 0;     // P_all_count
    int inUse    = 0;     // P_now_count
    int reserved = 0;     // P_reserve_count
    int free     = 0;     // total - inUse - reserved
    double fee   = 0.0;   // 单价
    bool   isFull() const { return free <= 0; }
};

/// 默认管理员账户（初始化时创建）
struct AdminAccount {
    QString username;
    QString password;   // 明文，写入数据库前 MD5
    QString truename;
    QString telephone;
};

Q_DECLARE_METATYPE(ParkingStatus)
