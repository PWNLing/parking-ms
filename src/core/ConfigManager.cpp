// core/ConfigManager.cpp
#include "ConfigManager.h"
#include "Logger.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

std::optional<AppConfig> ConfigManager::load(const QString& path) {
    QFile f(path);
    if (!f.exists()) {
        Logger::log(QtInfoMsg, QString("Config file not found: %1").arg(path));
        return std::nullopt;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        Logger::log(QtWarningMsg, QString("Cannot open config: %1").arg(path));
        return std::nullopt;
    }
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError) {
        Logger::log(QtWarningMsg, QString("Config parse error: %1").arg(err.errorString()));
        return std::nullopt;
    }
    auto obj = doc.object();
    AppConfig cfg;
    cfg.dbPath              = obj.value("db_path").toString("data/parking.db");
    cfg.parkingName         = obj.value("parking_name").toString();
    auto hlpr               = obj.value("hyperlpr3").toObject();
    cfg.hyperlpr3ModelsPath = hlpr.value("models_path").toString();
    cfg.hyperlpr3LibPath    = hlpr.value("lib_path").toString();
    cfg.initialized         = obj.value("initialized").toBool(false);
    cfg.version             = obj.value("version").toInt(1);

    cfg_ = cfg;
    Logger::log(QtInfoMsg, QString("Config loaded: parking=%1, db=%2, initialized=%3")
                .arg(cfg.parkingName, cfg.dbPath).arg(cfg.initialized));
    return cfg;
}

bool ConfigManager::save(const AppConfig& cfg, const QString& path) {
    QJsonObject obj;
    obj["db_path"]      = cfg.dbPath;
    obj["parking_name"] = cfg.parkingName;
    obj["initialized"]  = cfg.initialized;
    obj["version"]      = cfg.version;
    QJsonObject hlpr;
    hlpr["models_path"] = cfg.hyperlpr3ModelsPath;
    hlpr["lib_path"]    = cfg.hyperlpr3LibPath;
    obj["hyperlpr3"]    = hlpr;

    QJsonDocument doc(obj);

    // 确保目录存在
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath().isEmpty() ? "." : fi.absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Logger::log(QtWarningMsg, QString("Cannot write config: %1").arg(path));
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    cfg_ = cfg;
    Logger::log(QtInfoMsg, QString("Config saved: %1").arg(path));
    return true;
}
