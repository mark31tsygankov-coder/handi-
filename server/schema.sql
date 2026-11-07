CREATE TABLE IF NOT EXISTS admins (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS licenses (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  license_key VARCHAR(32) NOT NULL UNIQUE,
  duration_days INT UNSIGNED NULL COMMENT 'NULL = навсегда',
  status ENUM('active','blocked') NOT NULL DEFAULT 'active',
  note VARCHAR(190) NOT NULL DEFAULT '',
  created_at DATETIME NOT NULL,
  activated_at DATETIME NULL,
  expires_at DATETIME NULL COMMENT 'NULL после активации = навсегда',
  device_id VARCHAR(128) NULL,
  device_name VARCHAR(128) NULL,
  last_check DATETIME NULL,
  last_ip VARCHAR(45) NULL,
  INDEX idx_status (status),
  INDEX idx_device (device_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS rate_limit (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  ip VARCHAR(45) NOT NULL,
  bucket VARCHAR(32) NOT NULL,
  created_at DATETIME NOT NULL,
  INDEX idx_rl (ip, bucket, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
