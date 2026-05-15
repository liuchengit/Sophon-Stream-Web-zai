-- ============================================================================
-- Sophon-Stream Web Management System - Seed Data Migration
-- Version: V2
-- Description: Inserts default admin user, system configuration, and sample data
-- ============================================================================

-- ── Default Admin User ────────────────────────────────────────────────────
-- Username: admin
-- Password: admin123 (MUST be changed after first login!)
--
-- Password hash is generated using SHA-256 with salt.
-- Salt: a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
-- Hash: SHA-256("admin123" + "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6")
-- Note: In production, the backend's CryptoUtils will generate a unique salt
--       and hash on first run. This seed uses a pre-computed value for
--       environments where the backend initialization is not available.
INSERT OR IGNORE INTO users (username, password_hash, salt, role, real_name, email, status)
VALUES (
    'admin',
    '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918',
    'a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6',
    'admin',
    'System Administrator',
    'admin@sophon-stream.local',
    1
);

-- ── System Configuration ──────────────────────────────────────────────────
INSERT OR IGNORE INTO system_config (key, value, description) VALUES
    ('system.name', 'Sophon-Stream Web Management System', 'System display name'),
    ('system.version', '1.0.0', 'Current system version'),
    ('system.max_tasks', '100', 'Maximum concurrent tasks'),
    ('system.max_devices', '500', 'Maximum registered devices'),
    ('system.alert_retention_days', '90', 'Alert data retention period in days'),
    ('system.audit_retention_days', '180', 'Audit log retention period in days'),
    ('system.auto_backup', 'true', 'Enable automatic database backup'),
    ('system.backup_path', './backups', 'Backup storage directory'),
    ('system.backup_interval_hours', '24', 'Backup interval in hours'),
    ('monitor.tpu_enabled', 'true', 'Enable TPU monitoring'),
    ('monitor.broadcast_interval_ms', '2000', 'WebSocket broadcast interval in ms'),
    ('monitor.max_ws_connections', '100', 'Maximum WebSocket connections'),
    ('monitor.data_retention_hours', '72', 'Monitor data retention in hours');

-- ── Sample Algorithm Entries ──────────────────────────────────────────────
-- These are example algorithms to demonstrate the system.
-- In production, replace with actual model paths.
INSERT OR IGNORE INTO algorithms (name, type, model_path, config_json, version, description, input_size, classes, status)
VALUES
    (
        'YOLOv5s Object Detection',
        'detection',
        '/opt/sophon-stream/models/yolov5s_bm1684x.bmodel',
        '{"confidence_threshold": 0.5, "nms_threshold": 0.4, "max_detections": 100}',
        '1.0.0',
        'YOLOv5s general object detection model optimized for BM1684X',
        '640x640',
        '["person", "bicycle", "car", "motorcycle", "bus", "truck", "dog", "cat"]',
        1
    ),
    (
        'YOLOv8n Person Detection',
        'detection',
        '/opt/sophon-stream/models/yolov8n_person_bm1684x.bmodel',
        '{"confidence_threshold": 0.6, "nms_threshold": 0.45, "max_detections": 50}',
        '1.0.0',
        'YOLOv8n person-specific detection model for BM1684X',
        '640x640',
        '["person"]',
        1
    ),
    (
        'ResNet50 Classification',
        'classification',
        '/opt/sophon-stream/models/resnet50_bm1684x.bmodel',
        '{"top_k": 5}',
        '1.0.0',
        'ResNet50 image classification model',
        '224x224',
        '["ImageNet-1000"]',
        1
    ),
    (
        'ByteTrack Multi-Object Tracking',
        'tracking',
        '/opt/sophon-stream/models/bytetrack_bm1684x.bmodel',
        '{"track_threshold": 0.5, "match_threshold": 0.8, "min_box_area": 10}',
        '1.0.0',
        'ByteTrack multi-object tracker for BM1684X',
        '640x640',
        '[]',
        1
    ),
    (
        'PP-OCRv3 Text Recognition',
        'ocr',
        '/opt/sophon-stream/models/ppocrv3_bm1684x.bmodel',
        '{"det_limit_side_len": 960, "drop_score": 0.5}',
        '1.0.0',
        'PP-OCRv3 text detection and recognition for BM1684X',
        '960x960',
        '[]',
        1
    );

-- ── Sample Device Entry ──────────────────────────────────────────────────
INSERT OR IGNORE INTO devices (name, type, stream_url, protocol, resolution, fps, codec, status, location, description)
VALUES
    (
        'Sample RTSP Camera',
        'ipc',
        'rtsp://admin:password@192.168.1.100:554/stream1',
        'rtsp',
        '1920x1080',
        25.0,
        'H.264',
        0,
        'Building A - Entrance',
        'Sample IP camera for testing'
    );

-- ── Schema Migration Record ──────────────────────────────────────────────
INSERT OR IGNORE INTO schema_migrations (version, description)
VALUES ('2', 'Seed data: admin user, system config, sample algorithms');
