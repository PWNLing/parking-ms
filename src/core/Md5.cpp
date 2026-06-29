// core/Md5.cpp
#include "Md5.h"
#include <QCryptographicHash>

namespace Md5 {

QString hash(const QString& text) {
    return QString::fromLatin1(
        QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString hashWithSalt(const QString& text, const QString& salt) {
    return hash(text + salt);
}

} // namespace Md5
