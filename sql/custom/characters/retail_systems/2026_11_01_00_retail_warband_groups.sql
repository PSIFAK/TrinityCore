-- Warband groups of the character select screen (CMSG_SETUP_WARBAND_GROUPS), per account
CREATE TABLE IF NOT EXISTS `retail_account_warband_group` (
  `account_id` int unsigned NOT NULL,
  `group_id` bigint unsigned NOT NULL,
  `order_index` tinyint unsigned NOT NULL DEFAULT '0',
  `warband_scene_id` int unsigned NOT NULL DEFAULT '0',
  `flags` int unsigned NOT NULL DEFAULT '0',
  `content_set_id` int NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`account_id`, `group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_account_warband_group_member` (
  `account_id` int unsigned NOT NULL,
  `group_id` bigint unsigned NOT NULL,
  `member_index` tinyint unsigned NOT NULL,
  `placement_id` int unsigned NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `content_set_id` int NOT NULL DEFAULT '0',
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`account_id`, `group_id`, `member_index`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
