-- Exile's Reach (maps 2175/2236): creature data fixes from the retail sniff ExileReach_Hunter_12.1.5 (12.1.5.70688, Alliance night elf hunter).
-- Report: build/exiles_reach/hunter/creature_report.md. Only values the sniff shows unambiguously (query responses or identical creates).

-- ---------------------------------------------------------------------------------------------------------------------------
-- 1. creature_template_difficulty: DifficultyID 0 rows with ContentTuningID 0 (VerifiedBuild 0 stubs). TC then sets level 1 and
--    level-1 health (e.g. Captain Garrick 245394: TC L1 / 234 HP, retail L10 / 1131 HP). ContentTuningID from the sniff's
--    UnitData.ContentTuningID; HealthModifier of these rows already equals retail HpMulti.
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 1421 WHERE `DifficultyID` = 0 AND `ContentTuningID` = 0 AND `Entry` IN
(245394, 245397, 245667, 245686, 165360); -- Captain Garrick x2, Lady Jaina Proudmoore x2, Alliance Survivor: retail L10
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 741 WHERE `DifficultyID` = 0 AND `ContentTuningID` = 0 AND `Entry` IN
(245399, 253758, 156526, 160964, 164868); -- Jaina (summon), Wrathion, Scout-o-Matic 5000 (vehicle), Freed Expedition Member, Pandaren Monk
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 957 WHERE `DifficultyID` = 0 AND `ContentTuningID` = 0 AND `Entry` IN
(157051, 164577); -- Alliance Sparring Partner x2: retail L5
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 2588 WHERE `DifficultyID` = 0 AND `ContentTuningID` = 0 AND `Entry` = 244389; -- Kalecgos: retail L80, 2955200 HP

-- Darkmaul Citadel runs in DifficultyID 150 (fallback 0); query response HealthScalingExpansion = 11 (no HP change at level 1)
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion` = 11 WHERE `DifficultyID` = 150 AND `Entry` IN (168738, 168740); -- Citadel Rat, Citadel Recluse (was 9)

-- ---------------------------------------------------------------------------------------------------------------------------
-- 2. creature_template: stub templates (faction 0, no UNIT_FLAG2_REGENERATE_POWER) and other template fields the sniff disagrees with.
--    Flags keep only bits TC accepts from the DB (UNIT_FLAG*_ALLOWED); runtime bits (CAN_SWIM, IN_COMBAT, ...) are left out.
--    Summons whose retail faction is the summoner's player faction (1/3/4) keep their template faction.
--    Classification 1 (Elite) for the dragons comes from the query response.
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 149917; -- Captain Garrick (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 1732, `unit_flags2` = 0x800 WHERE `entry` = 150245; -- Lindie Springstock (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 151088; -- Kee-La (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 151089; -- Bjorn Stouthands (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 154170; -- Austin Huxworth (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 155125; -- InvisBunny (was: unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 155572; -- InvisBunny (was: unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x100, `unit_flags2` = 0x800, `unit_flags3` = 0x1000000, `speed_run` = 1 WHERE `entry` = 155733; -- Hidden Treasure Chest (was: unit_flags 0x0, unit_flags2 0x0, unit_flags3 0x0, speed_run 1.14286)
UPDATE `creature_template` SET `faction` = 7, `unit_flags` = 0x2000300, `unit_flags2` = 0x800, `unit_flags3` = 0x1000000 WHERE `entry` = 156403; -- aBoat (was: faction 0, unit_flags 0x0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `speed_run` = 1 WHERE `entry` = 156518; -- Scout-o-Matic 5000 (was: faction 0, speed_run 1.14286)
UPDATE `creature_template` SET `speed_run` = 1 WHERE `entry` = 156526; -- Scout-o-Matic 5000 (was: speed_run 1.14286)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x10 WHERE `entry` = 156609; -- Bjorn Stouthands (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x10 WHERE `entry` = 156610; -- Austin Huxworth (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x10 WHERE `entry` = 156612; -- Kee-La (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 156622; -- Quartermaster Richter (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 1732 WHERE `entry` = 156626; -- Lady Jaina Proudmoore (was: faction 0)
UPDATE `creature_template` SET `npcflag` = 0x1280, `unit_flags2` = 0x800 WHERE `entry` = 156800; -- Quartermaster Richter (was: npcflag 0x281, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `npcflag` = 0x2, `unit_flags2` = 0x800 WHERE `entry` = 156801; -- Private Cole (was: faction 0, npcflag 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 156808; -- Small Boar (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x300, `unit_flags2` = 0x800 WHERE `entry` = 156882; -- Meredy Huntswell (was: unit_flags 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 1732 WHERE `entry` = 156886; -- Meredy Huntswell (was: faction 35)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 156929; -- Ralia Dreamchaser (was: unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 156961; -- Lady Jaina Proudmoore (was: unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x300, `unit_flags2` = 0x800 WHERE `entry` = 156965; -- Captain Kelra (was: unit_flags 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x2000040 WHERE `entry` = 157051; -- Alliance Sparring Partner (was: unit_flags 0x40)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 160451; -- Hrun the Exiled (was: unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 1732, `unit_flags` = 0x300, `unit_flags2` = 0x800, `unit_flags3` = 0x80000 WHERE `entry` = 160964; -- Freed Expedition Member (was: faction 0, unit_flags 0x0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 161350; -- Captain Garrick (was: unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 162954; -- InvisBunny (was: unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 164022; -- InvisBunny (was: unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 164026; -- InvisBunny (was: unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x80030 WHERE `entry` = 164577; -- Alliance Sparring Partner (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x80020 WHERE `entry` = 164868; -- Pandaren Monk (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x2000200, `unit_flags2` = 0x800, `unit_flags3` = 0x1080000, `speed_run` = 1 WHERE `entry` = 164986; -- Freezing Trap (was: unit_flags 0x0, unit_flags2 0x0, unit_flags3 0x0, speed_run 1.14286)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800, `unit_flags3` = 0x80000 WHERE `entry` = 164990; -- Stalking Prowler (was: faction 0, unit_flags2 0x0, unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 165360; -- Alliance Survivor (was: unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 35, `unit_flags2` = 0x800 WHERE `entry` = 167347; -- Tiger (was: faction 0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 168382; -- Spar Point Advertisement (was: unit_flags3 0x0)
UPDATE `creature_template` SET `unit_flags3` = 0x1000000 WHERE `entry` = 174977; -- Spar Point Advertisement (was: unit_flags3 0x0)
UPDATE `creature_template` SET `Classification` = 1, `faction` = 3323, `unit_flags` = 0x40, `unit_flags2` = 0x800 WHERE `entry` = 244389; -- Kalecgos (was: Classification 0, faction 35, unit_flags 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 245394; -- Captain Garrick (was: unit_flags2 0x0)
UPDATE `creature_template` SET `faction` = 1732, `unit_flags` = 0x40, `unit_flags2` = 0x800 WHERE `entry` = 245397; -- Lady Jaina Proudmoore (was: faction 35, unit_flags 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags` = 0x300, `unit_flags2` = 0x800 WHERE `entry` = 245399; -- Lady Jaina Proudmoore (was: unit_flags 0x0, unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 245667; -- Captain Garrick (was: unit_flags2 0x0)
UPDATE `creature_template` SET `unit_flags2` = 0x800 WHERE `entry` = 245686; -- Lady Jaina Proudmoore (was: unit_flags2 0x0)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 248563; -- Wrathion (was: Classification 0)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 248565; -- Kalecgos (was: Classification 0)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 253710; -- Wrathion (was: Classification 0)
UPDATE `creature_template` SET `Classification` = 1, `faction` = 1665, `unit_flags` = 0x300, `unit_flags2` = 0x800 WHERE `entry` = 253758; -- Wrathion (was: Classification 0, faction 0, unit_flags 0x0, unit_flags2 0x0)

-- ---------------------------------------------------------------------------------------------------------------------------
-- 3. Equipment (VirtualItems of every sample)
-- Lady Jaina Proudmoore carries a staff (153575) on retail, the DB gives her Garrick's sword and shield
UPDATE `creature_equip_template` SET `ItemID1` = 153575, `AppearanceModID1` = 0, `ItemVisual1` = 0, `ItemID2` = 0, `AppearanceModID2` = 0, `ItemVisual2` = 0,
`ItemID3` = 0, `AppearanceModID3` = 0, `ItemVisual3` = 0, `VerifiedBuild` = 70688 WHERE `CreatureID` IN (156626, 156807, 156961) AND `ID` = 1;
UPDATE `creature_equip_template` SET `ItemID1` = 134066, `VerifiedBuild` = 70688 WHERE `CreatureID` = 156814 AND `ID` = 1; -- Gor'groth (Darkmaul Citadel), was 176578
UPDATE `creature_equip_template` SET `ItemID1` = 163883, `VerifiedBuild` = 70688 WHERE `CreatureID` = 165360 AND `ID` = 1; -- Alliance Survivor, was 163887 (offhand 163891 unchanged)
DELETE FROM `creature_equip_template` WHERE `CreatureID` IN (245394, 245397, 245667, 245686, 164577) AND `ID` = 1;
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, `ItemID2`, `AppearanceModID2`, `ItemVisual2`, `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES
(245394, 1, 163887, 0, 0, 163891, 0, 0, 0, 0, 0, 70688), -- Captain Garrick
(245667, 1, 163887, 0, 0, 163891, 0, 0, 0, 0, 0, 70688), -- Captain Garrick
(245397, 1, 153575, 0, 0, 0, 0, 0, 0, 0, 0, 70688), -- Lady Jaina Proudmoore
(245686, 1, 153575, 0, 0, 0, 0, 0, 0, 0, 0, 70688), -- Lady Jaina Proudmoore
(164577, 1, 108493, 0, 0, 0, 0, 0, 0, 0, 0, 70688); -- Alliance Sparring Partner (summon)
-- spawns of these entries have equipment_id 0 (= no weapons) although retail shows them armed
UPDATE `creature` SET `equipment_id` = 1 WHERE `guid` IN (8000044, 90010007, 90010000, 90010004, 90010006, 90010011) AND `equipment_id` = 0;
-- 8000044 Jaina 156807, 90010007 Jaina 156961, 90010000 Garrick 245394, 90010004 Jaina 245397, 90010006 Garrick 245667, 90010011 Jaina 245686

-- ---------------------------------------------------------------------------------------------------------------------------
-- 4. Spawns / addons
-- Austin Huxworth (hunter trainer / stable master at the camp below the Ogre Ruins): retail stands 6.2 yd from the DB spot
-- (same orientation, 5 creates, all identical; his private copy 164989 for The Art of Taming appears at the same spot)
UPDATE `creature` SET `position_x` = 183.23785, `position_y` = -2295.269, `position_z` = 82.24422, `orientation` = 0.230253174901008605, `VerifiedBuild` = 70688
WHERE `guid` = 1051924 AND `id` = 161666;
-- Ogre Shadowcaster: weapon drawn on 40 of 41 creates
UPDATE `creature_template_addon` SET `SheathState` = 1 WHERE `entry` = 153242 AND `SheathState` = 0;

-- Stalking Prowler (164990, quest 59356) is summoned with content tuning 482 (1-90): TC gave it level 90, retail level 10 / 377 HP
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 1421 WHERE `Entry` = 164990 AND `DifficultyID` IN (0, 1);

-- Jaina on the ship (156280): retail only shows her staff. Equipment 1 still held Garrick's sword and shield, which her
-- personal copies (summoned without an equipment id) took
UPDATE `creature_equip_template` SET `ItemID1` = 153575, `AppearanceModID1` = 0, `ItemVisual1` = 0, `ItemID2` = 0, `AppearanceModID2` = 0, `ItemVisual2` = 0,
`VerifiedBuild` = 70688 WHERE `CreatureID` = 156280 AND `ID` = 1;

-- Two campfires share the spot of the Cooking Meat camp (600228 Campfire 339769, 600251 Campfire 349962). Retail never
-- shows 349962 to the Alliance, and with both visible one of them popped in and out on phase changes
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 8 AND `SourceEntry` = 349962;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(32, 8, 349962, 0, 0, 6, 0, 67, 0, 0, 0, 0, 0, '', 'Campfire 349962 (second campfire of the Cooking Meat camp) is visible to the Horde only');

-- Jaina at the camp below the Ogre Ruins (156807) works against the ritual: retail (12.1.5 sniff) has her cast
-- Channeling (DNT) every 2 s from the end of the boar ride on
DELETE FROM `smart_scripts` WHERE `entryorguid` = 156807 AND `source_type` = 0 AND `id` = 2;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES
(156807, 0, 2, 0, 1, 0, 100, 0, 1000, 1000, 2000, 2000, 11, 1239043, 0, 1, 'Lady Jaina Proudmoore - Out of Combat - Cast Channeling (DNT)');
-- where the channel goes (retail's destination of every cast)
DELETE FROM `spell_target_position` WHERE `ID` = 1239043;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(1239043, 0, 0, 2175, 846.48956, -1609.8959, 593.677, 0, 70688);

-- Jaina's channel is the aura Channeling (DNT) 1239042 (periodic trigger of 1239043 every 2 s; every retail cast of
-- 1239043 carries it as OriginalCastID). The aura brings the channel animation, a bare cast of 1239043 has none.
DELETE FROM `smart_scripts` WHERE `entryorguid` = 156807 AND `source_type` = 0 AND `id` = 2;
DELETE FROM `creature_addon` WHERE `guid` = 8000044;
INSERT INTO `creature_addon` (`guid`, `auras`) VALUES
(8000044, '1239042');
UPDATE `creature` SET `position_x` = 188.85938, `position_y` = -2290.2275, `position_z` = 81.69565 WHERE `guid` = 8000044;

-- The Rescue of Meredy Huntswell (55763), from the retail (12.1.5) sniff: talking to the captive Meredy (menu 24887) starts
-- personal waves of Harpy Ambushers and Hunting Worgs, Kee-La and Henry Garrick come to help and Bloodbeak (153964) flies in
-- from the nest last (npc_exiles_reach_meredy_captive). Bloodbeak has no world spawn on retail.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 5 AND `SourceEntry` = 153964;
DELETE FROM `waypoint_path_node` WHERE `PathId` = 217500038;
DELETE FROM `waypoint_path` WHERE `PathId` = 217500038;
DELETE FROM `creature_addon` WHERE `guid` = 217500000038;
DELETE FROM `creature` WHERE `guid` = 217500000038;
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_meredy_ambush_ally' WHERE `entry` IN (155197, 155199);

-- the sniff only carries the ruRU lines without broadcast text ids (and the local BroadcastText.db2 lacks them), so the enUS
-- text is a translation
DELETE FROM `creature_text` WHERE (`CreatureID` = 153211 AND `GroupID` = 1) OR `CreatureID` = 155192;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(153211, 1, 0, 'I''ll break free with my magic. You just keep the harpies off me!', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Meredy Huntswell to Player - ambush start'),
(155192, 0, 0, 'Just... one... bite...', 12, 0, 50, 0, 0, 0, 0, 0, 0, 'Harpy Ambusher to Player'),
(155192, 0, 1, 'Just... a little... bite...', 12, 0, 50, 0, 0, 0, 0, 0, 0, 'Harpy Ambusher to Player'),
(155192, 1, 0, 'Dinner is here!', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Harpy Ambusher to Player');
DELETE FROM `creature_text_locale` WHERE ((`CreatureID` = 153211 AND `GroupID` = 1) OR `CreatureID` = 155192) AND `Locale` = 'ruRU';
INSERT INTO `creature_text_locale` (`CreatureID`, `GroupID`, `ID`, `Locale`, `Text`) VALUES
(153211, 1, 0, 'ruRU', 'Я освобожусь с помощью магии. Вы только гарпий отгоняйте!'),
(155192, 0, 0, 'ruRU', 'Мне бы... только... кусочек...'),
(155192, 0, 1, 'ruRU', 'Мне бы... хоть кусочек...'),
(155192, 1, 0, 'ruRU', 'А вот и ужин!');

-- the Harpy Ambushers, Hunting Worgs, Henry Garrick (155197) and Kee-La (155199) of the Meredy rescue were imported as world
-- spawns at the exact points where retail summons them for the event; the event summons them now, so they were duplicates
DELETE FROM `creature_addon` WHERE `guid` IN (SELECT `guid` FROM `creature` WHERE `id` IN (155192, 155197, 155199, 169162));
DELETE FROM `creature` WHERE `id` IN (155192, 155197, 155199, 169162);
