-- Exile's Reach: the Alliance hunter path from the retail sniff (12.1.5.70688, night elf hunter).
-- Scripts: zone_exiles_reach_human_mage.cpp (hunter section).

-- Northbound (55173) asked for Enhanced Combat Tactics (59254), a quest hunters never get: retail gives hunters
-- Northbound right after Cooking Meat (55174)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` = 55173 AND `ElseGroup` = 1;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(19, 0, 55173, 0, 1, 47, 0, 55174, 64, 0, 0, 'Northbound requires Cooking Meat rewarded (hunter)'),
(19, 0, 55173, 0, 1, 15, 0, 516, 0, 0, 0, 'Northbound: hunters and monks (no Enhanced Combat Tactics for them) skip it');
-- PrevQuestID 59254 blocked them on its own: the conditions above already ask for 59254 where it exists
UPDATE `quest_template_addon` SET `PrevQuestID` = 55174 WHERE `ID` = 55173;

-- the hunter quests at the camp below the Ogre Ruins, given and taken by Austin Huxworth after Freeing the Light
UPDATE `quest_template_addon` SET `PrevQuestID` = 55882 WHERE `ID` = 59355;
UPDATE `quest_template_addon` SET `PrevQuestID` = 59355 WHERE `ID` = 59356;
UPDATE `quest_template_addon` SET `PrevQuestID` = 59356 WHERE `ID` = 60168;
UPDATE `quest_template_addon` SET `ScriptName` = 'q59356_hunting_the_stalker' WHERE `ID` = 59356;

DELETE FROM `creature_queststarter` WHERE `quest` IN (59355, 59356, 60168);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(161666, 59355, 70688),
(161666, 59356, 70688),
(161666, 60168, 70688);
DELETE FROM `creature_questender` WHERE `quest` IN (59355, 59356, 60168);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(161666, 59355, 70688),
(161666, 59356, 70688),
(161666, 60168, 70688);

-- To Darkmaul Citadel (56344) waited for the mage's last class quest only: mages after 59354, hunters
-- after 60168, the other classes right after Freeing the Light
UPDATE `quest_template_addon` SET `PrevQuestID` = 55882 WHERE `ID` = 56344;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` = 56344;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(19, 0, 56344, 0, 0, 15, 0, 132, 0, 0, 1, 'Quest 56344: no class quest for this class'),
(19, 0, 56344, 0, 1, 15, 0, 128, 0, 0, 0, 'Quest 56344: mage'),
(19, 0, 56344, 0, 1, 8, 0, 59354, 0, 0, 0, 'Quest 56344: mage after The Art of Taming Sheep'),
(19, 0, 56344, 0, 2, 15, 0, 4, 0, 0, 0, 'Quest 56344: hunter'),
(19, 0, 56344, 0, 2, 8, 0, 60168, 0, 0, 0, 'Quest 56344: hunter after The Art of Taming');

UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_austin_huxworth_pet_trainer' WHERE `entry` = 161666;
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_freezing_trap' WHERE `entry` = 164986;
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_austin_huxworth_trap' WHERE `entry` = 164989;
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_stalking_prowler' WHERE `entry` = 164990;

-- A Hunter's Trap (59355): Meredy's ice mote (retail option 51803) ...
DELETE FROM `gossip_menu_option` WHERE `MenuID` = 25321 AND `OptionID` = 1;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(25321, 51803, 1, 0, 'Мереди, у тебя не найдется частицы льда?', 0, 0, 1, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 70688);
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 25321 AND `SourceEntry` = 1;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(15, 25321, 1, 0, 0, 47, 0, 59355, 8, 0, 0, 'Show the ice mote while A Hunter''s Trap is incomplete'),
(15, 25321, 1, 0, 0, 48, 0, 396411, 0, 1, 1, 'Hide the ice mote once Meredy gave it');

-- ... and the two trap parts by the camp: quest-only loot, a sparkle while the objective is open, gone once looted
-- (spawn trackings 1922439 / 1922440 of the sniff)
SET @OGUID := 217500100000; -- after the spawns file range (217500000000-217500099999, which it deletes)
-- an earlier version of this file used 217500000017/18, inside the spawns file's range
DELETE FROM `gameobject` WHERE `guid` IN (217500000017, 217500000018) AND `id` IN (346277, 346278);
DELETE FROM `spawn_tracking_state` WHERE `SpawnType` = 1 AND `SpawnId` IN (217500000017, 217500000018);
DELETE FROM `gameobject` WHERE `guid` IN (@OGUID+0, @OGUID+1);
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `ScriptName`, `StringId`, `VerifiedBuild`) VALUES
(@OGUID+0, 346278, 2175, 10424, 10588, '0', 0, 0, 0, -1, 238.17708, -2285.4844, 80.89585, 3.9419427, 0, 0, -0.92099285, 0.38957945, 30, 255, 1, '', NULL, 70688), -- Rusted Chains
(@OGUID+1, 346277, 2175, 10424, 10588, '0', 0, 0, 0, -1, 270.03125, -2270.9932, 80.87531, 4.715575, 0, 0, -0.70597935, 0.7082324, 30, 255, 1, '', NULL, 70688); -- Old Spring

UPDATE `gameobject_template` SET `Data1` = 346277 WHERE `entry` = 346277;
UPDATE `gameobject_template` SET `Data1` = 346278 WHERE `entry` = 346278;
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (346277, 346278);
INSERT INTO `gameobject_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(346277, 0, 175980, 100, 1, 1, 0, 1, 1, 'Old Spring - A Hunter''s Trap'),
(346278, 0, 175981, 100, 1, 1, 0, 1, 1, 'Rusted Chains - A Hunter''s Trap');

DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN (1922439, 1922440);
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(1922439, 1, @OGUID+0, '396412'),
(1922440, 1, @OGUID+1, '396413');
DELETE FROM `spawn_tracking_state` WHERE `SpawnType` = 1 AND `SpawnId` IN (@OGUID+0, @OGUID+1);
INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`, `StateSpellVisualId`, `StateAnimId`, `StateAnimKitId`, `StateWorldEffects`) VALUES
(1, @OGUID+0, 0, 0, NULL, NULL, NULL, ''),
(1, @OGUID+0, 1, 1, NULL, 1866, NULL, '29849'),
(1, @OGUID+0, 2, 0, NULL, NULL, NULL, ''),
(1, @OGUID+1, 0, 0, NULL, NULL, NULL, ''),
(1, @OGUID+1, 1, 1, NULL, 1866, NULL, '29850'),
(1, @OGUID+1, 2, 0, NULL, NULL, NULL, '');

-- The Art of Taming (60168): Austin calls the pet out only while it is still to be tamed
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 24886 AND `SourceEntry` = 2;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(15, 24886, 2, 0, 0, 47, 0, 60168, 8, 0, 0, 'Show the taming option while The Art of Taming is incomplete'),
(15, 24886, 2, 0, 0, 48, 0, 397752, 0, 1, 1, 'Hide the taming option once the new pet is tamed');

-- Austin's lines while the stalker walks into the trap; the actors are summons the script adds
DELETE FROM `conversation_template` WHERE `Id` IN (13707, 16330, 16331, 16332);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `TextureKitId`, `Flags`, `VerifiedBuild`) VALUES
(13707, 34159, 0, 0, 70688),
(16330, 41001, 0, 0, 70688),
(16331, 41003, 0, 0, 70688),
(16332, 41005, 0, 0, 70688);
DELETE FROM `conversation_line_template` WHERE `Id` IN (34159, 41001, 41003, 41005);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `Flags`, `ChatType`, `VerifiedBuild`) VALUES
(34159, 0, 0, 0, 0, 70688),
(41001, 0, 0, 0, 0, 70688),
(41003, 0, 0, 0, 0, 70688),
(41005, 0, 0, 0, 0, 70688);
