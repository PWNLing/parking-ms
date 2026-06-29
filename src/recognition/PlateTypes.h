// recognition/PlateTypes.h — 识别结果结构与类型映射
#pragma once
#include <QString>
#include <QRectF>
#include <QMetaType>
#include <vector>
#include <cstdint>

/// 车牌识别结果
struct PlateResult {
    QString code;            // 车牌号字符串，如 "津B6H920"
    int     type = -1;       // HLPR_PlateType 枚举值（0=蓝牌, 3=绿牌...）
    QString typeName;        // 中文类型名："蓝牌"/"绿牌新能源"...
    float   confidence = 0;  // 文本置信度 0~1
    QRectF  box;             // 检测框 (x1,y1,x2,y2)
    qint64  timestamp = 0;   // 识别时间戳 ms
};

Q_DECLARE_METATYPE(PlateResult)

/// 车牌类型中文名映射（对齐 hyper_lpr_sdk.h HLPR_PlateType 枚举顺序）
extern const std::vector<QString> PLATE_TYPE_NAMES;

/// 根据枚举值获取中文名
inline QString plateTypeName(int type) {
    if (type < 0 || type >= static_cast<int>(PLATE_TYPE_NAMES.size())) return "未知";
    return PLATE_TYPE_NAMES[type];
}
