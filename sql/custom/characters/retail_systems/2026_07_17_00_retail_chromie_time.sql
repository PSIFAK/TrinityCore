CREATE TABLE IF NOT EXISTS `retail_character_chromie_time` (
  `guid` bigint unsigned NOT NULL,
  `expansion_id` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
