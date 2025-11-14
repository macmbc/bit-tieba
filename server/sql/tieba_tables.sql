-- 贴吧系统数据库表结构
-- 使用数据库: llfc

-- 1. 贴吧表
CREATE TABLE IF NOT EXISTS `tieba` (
  `tieba_id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY COMMENT '贴吧ID',
  `tieba_name` VARCHAR(50) NOT NULL UNIQUE COMMENT '贴吧名称',
  `tieba_desc` TEXT COMMENT '贴吧描述',
  `tieba_icon` VARCHAR(255) DEFAULT '' COMMENT '贴吧图标',
  `owner_uid` INT NOT NULL COMMENT '吧主UID',
  `member_count` INT DEFAULT 0 COMMENT '成员数量',
  `post_count` INT DEFAULT 0 COMMENT '帖子数量',
  `create_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `update_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  INDEX `idx_owner` (`owner_uid`),
  INDEX `idx_name` (`tieba_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='贴吧表';

-- 2. 贴吧成员表
CREATE TABLE IF NOT EXISTS `tieba_member` (
  `id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  `tieba_id` INT NOT NULL COMMENT '贴吧ID',
  `uid` INT NOT NULL COMMENT '用户ID',
  `role` TINYINT DEFAULT 0 COMMENT '角色: 0-普通成员, 1-小吧主, 2-吧主',
  `join_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
  UNIQUE KEY `uk_tieba_uid` (`tieba_id`, `uid`),
  INDEX `idx_uid` (`uid`),
  INDEX `idx_tieba` (`tieba_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='贴吧成员表';

-- 3. 帖子表
CREATE TABLE IF NOT EXISTS `tieba_post` (
  `post_id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY COMMENT '帖子ID',
  `tieba_id` INT NOT NULL COMMENT '贴吧ID',
  `uid` INT NOT NULL COMMENT '发帖人UID',
  `title` VARCHAR(200) NOT NULL COMMENT '帖子标题',
  `content` TEXT NOT NULL COMMENT '帖子内容',
  `reply_count` INT DEFAULT 0 COMMENT '回复数量',
  `like_count` INT DEFAULT 0 COMMENT '点赞数量',
  `is_top` TINYINT DEFAULT 0 COMMENT '是否置顶: 0-否, 1-是',
  `is_essence` TINYINT DEFAULT 0 COMMENT '是否加精: 0-否, 1-是',
  `create_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '发帖时间',
  `update_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  INDEX `idx_tieba` (`tieba_id`),
  INDEX `idx_uid` (`uid`),
  INDEX `idx_time` (`create_time`),
  INDEX `idx_top` (`tieba_id`, `is_top`, `create_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='帖子表';

-- 4. 回复表
CREATE TABLE IF NOT EXISTS `tieba_reply` (
  `reply_id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY COMMENT '回复ID',
  `post_id` INT NOT NULL COMMENT '帖子ID',
  `uid` INT NOT NULL COMMENT '回复人UID',
  `content` TEXT NOT NULL COMMENT '回复内容',
  `reply_to_uid` INT DEFAULT 0 COMMENT '回复目标用户ID(0表示回复楼主)',
  `reply_to_reply_id` INT DEFAULT 0 COMMENT '回复目标回复ID(0表示回复楼主)',
  `like_count` INT DEFAULT 0 COMMENT '点赞数量',
  `create_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '回复时间',
  INDEX `idx_post` (`post_id`),
  INDEX `idx_uid` (`uid`),
  INDEX `idx_time` (`create_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='回复表';

-- 5. 点赞表
CREATE TABLE IF NOT EXISTS `tieba_like` (
  `id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  `uid` INT NOT NULL COMMENT '用户ID',
  `target_type` TINYINT NOT NULL COMMENT '目标类型: 1-帖子, 2-回复',
  `target_id` INT NOT NULL COMMENT '目标ID(帖子ID或回复ID)',
  `create_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '点赞时间',
  UNIQUE KEY `uk_like` (`uid`, `target_type`, `target_id`),
  INDEX `idx_target` (`target_type`, `target_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='点赞表';

-- 6. 贴吧ID自增表
CREATE TABLE IF NOT EXISTS `tieba_id` (
  `id` INT NOT NULL DEFAULT 1 PRIMARY KEY
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='贴吧ID自增表';

-- 初始化贴吧ID表
INSERT INTO `tieba_id` (`id`) VALUES (1) ON DUPLICATE KEY UPDATE `id` = `id`;

-- 7. 帖子ID自增表
CREATE TABLE IF NOT EXISTS `post_id` (
  `id` INT NOT NULL DEFAULT 1 PRIMARY KEY
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='帖子ID自增表';

-- 初始化帖子ID表
INSERT INTO `post_id` (`id`) VALUES (1) ON DUPLICATE KEY UPDATE `id` = `id`;

-- 8. 回复ID自增表
CREATE TABLE IF NOT EXISTS `reply_id` (
  `id` INT NOT NULL DEFAULT 1 PRIMARY KEY
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='回复ID自增表';

-- 初始化回复ID表
INSERT INTO `reply_id` (`id`) VALUES (1) ON DUPLICATE KEY UPDATE `id` = `id`;

