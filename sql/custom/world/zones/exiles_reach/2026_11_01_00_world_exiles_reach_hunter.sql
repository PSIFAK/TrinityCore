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

-- The Harpy Problem (55196): retail (12.1.5 sniff) has Henry Garrick offer it as soon as Stocking Up on Supplies (55194)
-- is turned in (quest giver status "journey quest" from that moment), not only after Repair Yourself (85678)
UPDATE `quest_template_addon` SET `PrevQuestID` = 55194 WHERE `ID` = 55196;

-- Who Lurks in the Pit (55639): the captive Ralia (156902) had phase 14328, which is not cosmetic in Phase.db2 and so
-- hid every unphased spawn of the pit (spiders, the five cocoons) while the quest ran. She keeps her retail timing
-- through spawn visibility conditions instead, without a phase.
DELETE FROM `phase_area` WHERE `PhaseId` = 14328 AND `AreaId` = 10527;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 14328 AND `SourceEntry` = 10527;
UPDATE `creature` SET `PhaseId` = 0 WHERE `id` = 156902 AND `PhaseId` = 14328;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 5 AND `SourceEntry` = 156902;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(32, 5, 156902, 0, 0, 47, 0, 55639, 8, 0, 0, 'Captive Ralia is visible while Who Lurks in the Pit is in progress'),
(32, 5, 156902, 0, 0, 48, 0, 391940, 0, 1, 1, 'Captive Ralia is visible until Hrun the Exiled is defeated');

-- Who Lurks in the Pit (55639): the expedition member freed from a Thick Cocoon appears where the cocoon was, says one
-- of four lines and runs for the way out (zone_exiles_reach_human_mage.cpp). The retail sniff has no broadcast text for
-- these lines: enUS is a translation of the ruRU text it showed.
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_freed_expedition_member' WHERE `entry` = 160964;
DELETE FROM `spell_script_names` WHERE `spell_id` = 313324;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(313324, 'spell_exiles_reach_freed_expedition_member');
DELETE FROM `creature_text` WHERE `CreatureID` = 160964;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(160964, 0, 0, 'Thank you! Ralia Dreamchaser tried to save us. She must be somewhere below.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Freed Expedition Member'),
(160964, 0, 1, 'I thought I was done for! Please, save Ralia Dreamchaser. She is down below.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Freed Expedition Member'),
(160964, 0, 2, 'I will be afraid of spiders for the rest of my life. Have you saved Ralia Dreamchaser yet?', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Freed Expedition Member'),
(160964, 0, 3, 'Save Ralia Dreamchaser! She is somewhere deep in the cave.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Freed Expedition Member');
DELETE FROM `creature_text_locale` WHERE `CreatureID` = 160964;
INSERT INTO `creature_text_locale` (`CreatureID`, `GroupID`, `ID`, `Locale`, `Text`) VALUES
(160964, 0, 0, 'ruRU', 'Благодарю тебя! Ралия Мечтательница пыталась спасти нас. Должно быть, она где-то внизу.'),
(160964, 0, 1, 'ruRU', 'Я думал, что мне конец! Пожалуйста, спаси Ралию Мечтательницу. Она внизу.'),
(160964, 0, 2, 'ruRU', 'Всю жизнь теперь буду бояться пауков. Тебе уже удалось спасти Ралию Мечтательницу?'),
(160964, 0, 3, 'ruRU', 'Спаси Ралию Мечтательницу! Она где-то в глубине пещеры.');

-- Retail (12.1.5 sniff): the freed Ralia (156929) and the two defeated Hrun (160451) only appear once Hrun the Exiled
-- is defeated, and leave when Ralia is rescued. Their phases (13834, 13816) are on for the whole quest, so spawn
-- conditions narrow them down
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 5 AND `SourceEntry` IN (156929, 160451);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(32, 5, 156929, 0, 0, 47, 0, 55639, 8, 0, 0, 'Freed Ralia (pit) is visible while Who Lurks in the Pit is in progress'),
(32, 5, 156929, 0, 0, 48, 0, 391940, 0, 1, 0, 'Freed Ralia (pit) is visible once Hrun the Exiled is defeated'),
(32, 5, 156929, 0, 0, 48, 0, 393740, 0, 1, 1, 'Freed Ralia (pit) is visible until she is rescued'),
(32, 5, 160451, 0, 0, 47, 0, 55639, 8, 0, 0, 'Defeated Hrun is visible while Who Lurks in the Pit is in progress'),
(32, 5, 160451, 0, 0, 48, 0, 391940, 0, 1, 0, 'Defeated Hrun is visible once Hrun the Exiled is defeated'),
(32, 5, 160451, 0, 0, 48, 0, 393740, 0, 1, 1, 'Defeated Hrun is visible until Ralia is rescued');

-- Freeing the Light (54933): retail (12.1.5 sniff) has Lightspawn give it while The Harpy Problem (55196) is still in
-- the log (taken 02:25:14, 54933 taken 02:25:53, 55196 turned in 02:27:02): it opens with Repair Yourself (85678)
UPDATE `quest_template_addon` SET `PrevQuestID` = 85678 WHERE `ID` = 54933;

-- the four ritual sources around Lightspawn are hostile invisible bunnies that must never fight the player
UPDATE `creature_template` SET `flags_extra` = `flags_extra` | 0x2 WHERE `entry` IN (155572, 164022, 164025, 164026);

-- Killclaw the Terrible (56839) is started by the Danger Sign gameobject (330627, retail 12.1.5 sniff: it appears with
-- To Darkmaul Citadel taken and gives the quest); the starter was in creature_queststarter, where no creature has that entry
DELETE FROM `creature_queststarter` WHERE `id` = 330627 AND `quest` = 56839;
DELETE FROM `gameobject_queststarter` WHERE `id` = 330627 AND `quest` = 56839;
INSERT INTO `gameobject_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(330627, 56839, 70688);
