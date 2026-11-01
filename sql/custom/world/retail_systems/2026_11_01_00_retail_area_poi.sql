-- Timed area POIs of the world map (CMSG_REQUEST_AREA_POI_UPDATE), RetailSystems AreaPoi.cpp.
-- An entry is active for duration_sec at the start of every period_sec, counted from first_start (unix time).
CREATE TABLE IF NOT EXISTS `retail_area_poi` (
  `area_poi_id` int NOT NULL,
  `first_start` bigint unsigned NOT NULL,
  `period_sec` int NOT NULL,
  `duration_sec` int NOT NULL,
  `world_state_variable_id` int unsigned NOT NULL DEFAULT '0',
  `world_state_value` int unsigned NOT NULL DEFAULT '0',
  `comment` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`area_poi_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- the POIs of the retail sniff 12.1.5.70688 (EU, 2026-10-31). Start times and durations are retail's; the periods
-- are guessed: weekly for the week long ones (reset), hourly for the short ones, 1.5 h for the three surges that
-- took turns on world state 30173
DELETE FROM `retail_area_poi`;
INSERT INTO `retail_area_poi` (`area_poi_id`, `first_start`, `period_sec`, `duration_sec`, `world_state_variable_id`, `world_state_value`, `comment`) VALUES
(5286, 1793488895, 3600, 1800, 12869, 1, 'Malorus the Soulkeeper'),
(5290, 1793485758, 3600, 1800, 12873, 1, 'Inquisitor Chillbane'),
(5295, 1793487723, 3600, 1800, 12878, 1, 'Corrupted Bonebreaker'),
(7556, 1793160008, 604800, 604800, 24157, 1, 'Dreamsurge'),
(7634, 1793487607, 3600, 600, 24932, 1, 'Superbloom'),
(8174, 1793487619, 3600, 900, 27057, 1, 'Nightfall'),
(8175, 1793484019, 3600, 3540, 27062, 1, 'Nightfall'),
(8263, 1793489451, 3600, 600, 27237, 1, 'Surge Pricing'),
(8758, 1793160017, 604800, 604800, 29528, 1, 'Impending Void Incursion'),
(8812, 1793160017, 604800, 604800, 29528, 1, 'Aqir Invasion of Zul''Aman'),
(8937, 1793485828, 5400, 1800, 30173, 4, 'Curse Surge: Siege at the Whispering Marsh'),
(8938, 1793489428, 5400, 1800, 30173, 2, 'Curse Surge: The Broodmother''s Nest'),
(8940, 1793487628, 5400, 1800, 30173, 5, 'Curse Surge: The Malformed Leviathan'),
(9026, 1793160017, 604800, 604800, 29528, 1, 'Aqir Invasion');
