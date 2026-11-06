-- Arrival on the Dragon Isles after Exile's Reach (Wild Coast, Waking Shores).

-- An End to Beginnings (55991): Play Movie, Load (1238030) teleports behind a loading screen; destination from the retail
-- 12.0.1 sniff (SMSG_TRANSFER_PENDING / SMSG_NEW_WORLD)
DELETE FROM `spell_target_position` WHERE `ID` = 1238030;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(1238030, 0, 0, 2444, 3713.47, -1890.45, 5.77, 2.7612855, 66709);

-- The Dragon Isles storyline (wowhead): What's Your Specialty? (87547) -> Home Is Where the Hearth Is (87555) ->
-- Aiding the Dragon Isles (87562), all from Kalecgos (238913); the last one is turned in to Toddy Whiskers (189022)
DELETE FROM `creature_questender` WHERE `quest` IN (87555, 87562);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(238913, 87555, 66709),
(189022, 87562, 66709);
DELETE FROM `creature_queststarter` WHERE `quest` = 87562;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(238913, 87562, 66709);
DELETE FROM `quest_template_addon` WHERE `ID` = 87562;
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES
(87562, 87555);

-- Toddy Whiskers had no spawn; wowhead places him at 82.0, 31.8 on The Waking Shores (UiMap 2022), next to the landing
UPDATE `creature_template` SET `faction` = 35, `npcflag` = 2 WHERE `entry` = 189022;
DELETE FROM `creature` WHERE `guid` = 217500000164;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `VerifiedBuild`) VALUES
(217500000164, 189022, 2444, 13644, 13722, '0', 25711, 0, 3680.9, -1888.4, 4.6, 6.0, 120, 0);
