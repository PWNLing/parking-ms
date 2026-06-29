// core/ConfigManager.h — config.json 读写（单例）
#pragma once
#include <QString>
#include <optional>

/// 应用配置（持久化到 config.json）
struct AppConfig {
    QString dbPath;                 // SQLite 数据库文件路径
    QString parkingName;            // 停车场名称
    QString hyperlpr3ModelsPath;    // HyperLPR3 模型目录
    QString hyperlpr3LibPath;       // HyperLPR3 库目录
    bool    initialized = false;    // 是否完成初始化
    int     version = 1;            // 配置版本
};

class ConfigManager {
public:
    static ConfigManager& instance();

    /// 从 JSON 文件加载配置；文件不存在返回 nullopt
    std::optional<AppConfig> load(const QString& path = "config.json");

    /// 保存配置到 JSON 文件
    bool save(const AppConfig& cfg, const QString& path = "config.json");

    /// 当前内存中的配置
    const AppConfig& current() const { return cfg_; }

    /// 设置当前配置（不持久化，需 save）
    void setCurrent(const AppConfig& cfg) { cfg_ = cfg; }

private:
    ConfigManager() = default;
    AppConfig cfg_;
};
