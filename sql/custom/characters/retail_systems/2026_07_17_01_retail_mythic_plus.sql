-- RetailSystems: Challenge Mode / Mythic+
-- Kept outside TrinityCore base tables so upstream character database updates remain merge-friendly.

CREATE TABLE IF NOT EXISTS `retail_mythic_keystone` (
  `item_guid` bigint unsigned NOT NULL,
  `owner_guid` bigint unsigned NOT NULL,
  `map_challenge_mode_id` int unsigned NOT NULL,
  `level` int unsigned NOT NULL DEFAULT '2',
  `affix_1` int NOT NULL DEFAULT '0',
  `affix_2` int NOT NULL DEFAULT '0',
  `affix_3` int NOT NULL DEFAULT '0',
  `affix_4` int NOT NULL DEFAULT '0',
  `updated_at` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`item_guid`),
  KEY `idx_retail_mythic_keystone_owner` (`owner_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_mythic_run` (
  `id` bigint unsigned NOT NULL,
  `map_id` int unsigned NOT NULL,
  `challenge_id` int unsigned NOT NULL,
  `level` int unsigned NOT NULL,
  `duration_ms` bigint unsigned NOT NULL,
  `start_time` bigint unsigned NOT NULL,
  `completion_time` bigint unsigned NOT NULL,
  `mythic_season_id` int unsigned NOT NULL,
  `display_season_id` int unsigned NOT NULL,
  `score` float NOT NULL DEFAULT '0',
  `affix_1` int NOT NULL DEFAULT '0',
  `affix_2` int NOT NULL DEFAULT '0',
  `affix_3` int NOT NULL DEFAULT '0',
  `affix_4` int NOT NULL DEFAULT '0',
  `death_count` int unsigned NOT NULL DEFAULT '0',
  `upgrade_level` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_retail_mythic_run_season_challenge` (`display_season_id`,`challenge_id`),
  KEY `idx_retail_mythic_run_completion` (`completion_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_mythic_run_member` (
  `run_id` bigint unsigned NOT NULL,
  `member_guid` bigint unsigned NOT NULL,
  `guild_id` bigint unsigned NOT NULL DEFAULT '0',
  `specialization_id` int NOT NULL DEFAULT '0',
  `race_id` tinyint NOT NULL DEFAULT '0',
  `item_level` int NOT NULL DEFAULT '0',
  `name` varchar(24) NOT NULL DEFAULT '',
  `eligible_for_score` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`run_id`,`member_guid`),
  KEY `idx_retail_mythic_run_member_guid` (`member_guid`,`run_id`),
  CONSTRAINT `fk_retail_mythic_run_member_run` FOREIGN KEY (`run_id`) REFERENCES `retail_mythic_run` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Optional operator override. When empty, the server derives a deterministic weekly
-- rotation from MythicPlusSeasonTrackedAffix.db2 for the active display season.
CREATE TABLE IF NOT EXISTS `retail_mythic_affix_rotation` (
  `display_season_id` int unsigned NOT NULL,
  `week_start` bigint unsigned NOT NULL,
  `affix_1` int NOT NULL DEFAULT '0',
  `affix_2` int NOT NULL DEFAULT '0',
  `affix_3` int NOT NULL DEFAULT '0',
  `affix_4` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`display_season_id`,`week_start`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
