-- ============================================================
-- 演示数据 (seed_demo.sql)
-- 用法: sqlite3 data/parking.db < scripts/seed_demo.sql
-- 注意: 执行前请确保数据库已通过初始化向导创建
--       密码均为 "123456" 的 MD5: e10adc3949ba59abbe56e057f20f883e
-- ============================================================

-- 清空现有数据（演示用）
DELETE FROM reservations;
DELETE FROM CAR;
DELETE FROM USER;
DELETE FROM PARKING;

-- 管理员账户
INSERT INTO USER(username, password, truename, telephone) VALUES
('admin', 'e10adc3949ba59abbe56e057f20f883e', '系统管理员', '13800000000'),
('zhang', 'e10adc3949ba59abbe56e057f20f883e', '张三',       '13800001111');

-- 停车场
INSERT INTO PARKING(P_name, P_reserve_count, P_now_count, P_all_count, P_fee) VALUES
('新工停车场', 1, 2, 100, 5.00);

-- 出入库记录
-- 1. 已结算记录（>30分钟，费用 10.00）
INSERT INTO CAR(license_plate, check_in_time, check_out_time, fee, location, unit_price) VALUES
('津B6H920', '2026-06-23 08:30:00', '2026-06-23 10:15:00', 10.00, '新工停车场', 5.00);

-- 2. 已结算记录（≤30分钟，免费）
INSERT INTO CAR(license_plate, check_in_time, check_out_time, fee, location, unit_price) VALUES
('皖KD01833', '2026-06-23 09:00:00', '2026-06-23 09:20:00', 0.00, '新工停车场', 5.00);

-- 3. 在场中记录（未结算）
INSERT INTO CAR(license_plate, check_in_time, check_out_time, fee, location, unit_price) VALUES
('蒙B023H6', '2026-06-23 13:00:00', NULL, NULL, '新工停车场', 5.00);

-- 4. 在场中记录（未结算）
INSERT INTO CAR(license_plate, check_in_time, check_out_time, fee, location, unit_price) VALUES
('冀D5L690', '2026-06-23 13:30:00', NULL, NULL, '新工停车场', 5.00);

-- 预约记录（会触发 P_reserve_count 同步）
INSERT INTO reservations(license_plate, P_name, created_at) VALUES
('京A12345', '新工停车场', datetime('now','localtime'));

-- 验证
SELECT '--- USER ---' AS info;
SELECT id, username, truename, telephone FROM USER;
SELECT '--- PARKING ---' AS info;
SELECT P_name, P_now_count, P_reserve_count, P_all_count, P_fee FROM PARKING;
SELECT '--- CAR ---' AS info;
SELECT id, license_plate, check_in_time, check_out_time, fee FROM CAR;
SELECT '--- reservations ---' AS info;
SELECT id, license_plate, P_name, created_at FROM reservations;
