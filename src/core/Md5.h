// core/Md5.h — MD5 哈希工具
#pragma once
#include <QString>

namespace Md5 {

/// 计算字符串的 MD5 哈希，返回 32 位小写十六进制串
QString hash(const QString& text);

/// 加盐 MD5（增强安全性，本项目暂用纯 MD5 对齐论文）
QString hashWithSalt(const QString& text, const QString& salt);

} // namespace Md5
