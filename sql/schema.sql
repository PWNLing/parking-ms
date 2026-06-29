-- ============================================================
-- 停车场管理信息系统 SQLite Schema
-- 版本: 1.0
-- 引擎: SQLite3 (内置于 Qt6 QSqlDatabase)
-- 对齐 design.md §4.2
-- ============================================================

-- 启用外键支持（每次连接需执行）
PRAGMA foreign_keys = ON;
-- WAL 模式提升并发读性能
PRAGMA journal_mode = WAL;

-- ------------------------------------------------------------
-- 表 1: USER 管理员/用户账户表
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS USER (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    username    TEXT    UNIQUE NOT NULL,
    password    TEXT    NOT NULL,                          -- MD5 32位小写十六进制
    truename    TEXT    NOT NULL,
    telephone   TEXT    NOT NULL CHECK (length(telephone) = 11)
);

-- ------------------------------------------------------------
-- 表 2: PARKING 停车场表
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS PARKING (
    P_id              INTEGER PRIMARY KEY AUTOINCREMENT,
    P_name            TEXT    UNIQUE NOT NULL,
    P_reserve_count   INTEGER NOT NULL DEFAULT 0 CHECK (P_reserve_count >= 0),
    P_now_count       INTEGER NOT NULL DEFAULT 0 CHECK (P_now_count >= 0),
    P_all_count       INTEGER NOT NULL CHECK (P_all_count > 0),
    P_fee             REAL    NOT NULL CHECK (P_fee > 0),
    CHECK (P_now_count + P_reserve_count <= P_all_count)
);

-- ------------------------------------------------------------
-- 表 3: CAR 车辆出入库记录表
-- （合并论文"汽车表"+"进出记录表"）
-- 修复论文 BUG：CAR 表 UNIQUE(license_plate) 会导致同车只能入库一次
--   → 改为部分索引 idx_car_inprogress（仅约束"在场中"的记录唯一）
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS CAR (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    license_plate   TEXT    NOT NULL,
    check_in_time   TEXT    NOT NULL,                       -- ISO8601 'YYYY-MM-DD HH:MM:SS'
    check_out_time  TEXT,                                    -- NULL = 在场中
    fee             REAL,                                    -- NULL = 未结算
    location        TEXT    NOT NULL,                       -- 停车场名（冗余便于查询）
    unit_price      REAL    NOT NULL,                       -- 入库时单价快照
    CHECK (fee IS NULL OR fee >= 0)
);
CREATE INDEX IF NOT EXISTS idx_car_plate        ON CAR(license_plate);
CREATE INDEX IF NOT EXISTS idx_car_checkin      ON CAR(check_in_time);
-- 部分索引：保证同车牌同时只能有一条"在场中"记录
CREATE UNIQUE INDEX IF NOT EXISTS idx_car_inprogress
    ON CAR(license_plate) WHERE check_out_time IS NULL;

-- ------------------------------------------------------------
-- 表 4: reservations 预约表
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS reservations (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    license_plate   TEXT    UNIQUE NOT NULL,                -- 同车牌不可重复预约
    P_name          TEXT    NOT NULL,
    created_at      TEXT    DEFAULT (datetime('now','localtime'))
);
CREATE INDEX IF NOT EXISTS idx_res_created ON reservations(created_at);
CREATE INDEX IF NOT EXISTS idx_res_pname   ON reservations(P_name);

-- ============================================================
-- 触发器
-- ============================================================

-- 触发器 1: 新增预约 → 停车场预约数 +1
CREATE TRIGGER IF NOT EXISTS trg_reserve_add
AFTER INSERT ON reservations
FOR EACH ROW
BEGIN
    UPDATE PARKING
    SET P_reserve_count = P_reserve_count + 1
    WHERE P_name = NEW.P_name;
END;

-- 触发器 2: 删除预约 → 停车场预约数 -1
CREATE TRIGGER IF NOT EXISTS trg_reserve_del
AFTER DELETE ON reservations
FOR EACH ROW
BEGIN
    UPDATE PARKING
    SET P_reserve_count = P_reserve_count - 1
    WHERE P_name = OLD.P_name;
END;

-- 触发器 3: 防止 P_now_count + P_reserve_count 超过 P_all_count（安全网）
CREATE TRIGGER IF NOT EXISTS trg_parking_capacity_guard
AFTER UPDATE OF P_now_count, P_reserve_count ON PARKING
FOR EACH ROW
WHEN NEW.P_now_count + NEW.P_reserve_count > NEW.P_all_count
BEGIN
    SELECT RAISE(ABORT, '停车场容量溢出');
END;

-- ============================================================
-- Schema 初始化完成
-- 种子数据由 InitWizard 写入（停车场 + 默认管理员）
-- ============================================================
