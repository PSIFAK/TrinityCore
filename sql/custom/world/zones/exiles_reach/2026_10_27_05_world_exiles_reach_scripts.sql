-- Exile's Reach: quest scripts and creature texts from the retail sniff (12.0.1.66709, Alliance human mage).
-- Conversations are started by src/server/scripts/ExilesReach/zone_exiles_reach_human_mage.cpp.

-- Quest scripts
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
(54951, 'q54951_emergency_first_aid'),
(55639, 'q55639_who_lurks_in_the_pit'),
(55764, 'q55764_harpy_culling'),
(59352, 'q59352_mage_knowledge'),
(59354, 'q59354_the_art_of_taming_sheep'),
(55991, 'q55991_the_end_of_the_beginning'),
(55981, 'q55981_the_enemy_of_my_enemy')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- Prisoners that follow the disguised player through the ogre camp (summoned by spell 298232)
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_ogre_camp_prisoner' WHERE `entry` IN (153565, 153566, 245399);

-- Creature texts heard in the sniff (texts are in 2026_10_27_01_world_exiles_reach_texts.sql)
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (152843, 153239, 153581, 153582, 153583, 156900, 156986, 153211, 156833, 156859) AND `ScriptName` = '';

DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` IN (152843, 153239, 153581, 153582, 153583, 156900, 156986, 153211, 156833, 156859);
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` = 156807 AND `id` = 1;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- death and aggro lines
(152843, 0, 0, 0, 6, 0, 50, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Harpy Boltcaller - On Death - Say Line 0'),
(153239, 0, 0, 0, 6, 0, 50, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ogre Brute - On Death - Say Line 0'),
(153581, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Grunk - On Death - Say Line 0'),
(153582, 0, 0, 0, 4, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Wug - On Aggro - Say Line 0'),
(153582, 0, 1, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Wug - On Death - Say Line 1'),
(153583, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Jugnug - On Death - Say Line 0'),
(156900, 0, 0, 0, 4, 0, 100, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Hrun the Exiled - On Aggro - Say Line 1'),
(156900, 0, 1, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Hrun the Exiled - On Death - Say Line 0'),
(156900, 0, 2, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 19, 156929, 50, 0, 0, 0, 0, 0, 'Hrun the Exiled - On Death - Ralia Dreamchaser Say Line 0'),
(156986, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ogre Taskmaster - On Death - Say Line 0'),
-- lines said when the player comes close to a phased-in story NPC
(153211, 0, 0, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Meredy Huntswell - On Player In LOS - Say Line 0'),
(156833, 0, 0, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Henry Garrick - On Player In LOS - Say Line 0'),
(156859, 0, 0, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Henry Garrick - On Player In LOS - Say Line 0'),
(156807, 0, 1, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Lady Jaina Proudmoore - On Player In LOS - Say Line 0');
