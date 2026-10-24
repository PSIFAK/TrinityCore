-- RetailSystems world quests: cycle in which a character completed a world quest, so it becomes available again after the reset
CREATE TABLE IF NOT EXISTS `retail_character_world_quest` (
  `guid` bigint unsigned NOT NULL,
  `QuestID` int unsigned NOT NULL,
  `CycleStart` bigint NOT NULL,
  PRIMARY KEY (`guid`,`QuestID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
