-- Exile's Reach: quest scripts and creature texts from the retail sniff (12.0.1.66709, Alliance human mage).
-- Conversations are started by src/server/scripts/ExilesReach/zone_exiles_reach_human_mage.cpp.

-- Quest scripts
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
(54951, 'q54951_emergency_first_aid'),
(55639, 'q55639_who_lurks_in_the_pit'),
(55763, 'q55763_the_rescue_of_meredy_huntswell'),
(55764, 'q55764_harpy_culling'),
(59352, 'q59352_mage_knowledge'),
(59354, 'q59354_the_art_of_taming_sheep'),
(55991, 'q55991_the_end_of_the_beginning'),
(55981, 'q55981_the_enemy_of_my_enemy')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- Prisoners that follow the disguised player through the ogre camp (summoned by spell 298232)
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_ogre_camp_prisoner' WHERE `entry` IN (153565, 153566, 245399);

-- Creature texts heard in the sniff (texts are in 2026_10_27_01_world_exiles_reach_texts.sql)
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (152843, 153239, 153581, 153582, 153583, 156900, 156986, 156833, 156859) AND `ScriptName` = '';

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
(156833, 0, 0, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Henry Garrick - On Player In LOS - Say Line 0'),
(156859, 0, 0, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Henry Garrick - On Player In LOS - Say Line 0'),
(156807, 0, 1, 0, 10, 0, 100, 0, 1, 30, 120000, 120000, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Lady Jaina Proudmoore - On Player In LOS - Say Line 0');

-- 54952 Finding the Lost Expedition: in 12.x Jaina stays on the beach (no follower) and the quest is turned in to Captain Garrick at the camp
DELETE FROM `creature_questender` WHERE `quest` = 54952;
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(156651, 54952, 66709);

-- conversation 12044 is started for the player who takes 54952; its second actor is sent by retail without an object
UPDATE `conversation_template` SET `Flags` = 4 WHERE `Id` = 12044;
DELETE FROM `conversation_actors` WHERE `ConversationId` = 12044;
INSERT INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `ConversationActorGuid`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(12044, 0, 0, 0, 0, 0, 0, 1, 66709);


-- Vehicles summoned on the island must seat the summoner: the effect value of 305523 (Scout-o-Matic 5000), 326624 (Choppy Booster)
-- and 305069 is the ride spell 173426 (Ride Vehicle), as retail casts it right after the summon.
-- Their SummonProperties (4693, 4694) are missing from the client data, see sql/custom/hotfixes/zones/exiles_reach.
INSERT INTO `summon_properties_addon` (`SummonPropertiesId`, `EffectValueType`) VALUES
(4693, 4),
(4694, 4)
ON DUPLICATE KEY UPDATE `EffectValueType` = VALUES(`EffectValueType`);

-- 55639 Who Lurks in the Pit: the Thick Cocoons were stored in their used state (IN_USE flag, state 0), so they could not be
-- targeted or opened. Retail sends them ready (flags 0x40000, state 1) with display 91251; IN_USE is set only after a rescue.
UPDATE `gameobject_template_addon` SET `flags` = 0x40000 WHERE `entry` = 339568;
UPDATE `gameobject_template` SET `displayId` = 91251 WHERE `entry` = 339568;
UPDATE `gameobject` SET `state` = 1 WHERE `id` = 339568 AND `map` = 2175;

-- 59352 Mage Knowledge: Meredy's Spellbook is ready (state 1) in retail, not already opened
UPDATE `gameobject` SET `state` = 1 WHERE `id` = 346273 AND `map` = 2175;

-- 55639 Who Lurks in the Pit: the rideable Ralia (156929, phase 13834) appears only once Hrun is defeated (objective 391940),
-- the captive Ralia (156902, phase 14328) only until then; before, both were visible together.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 13834 AND `SourceEntry` = 10527 AND `ConditionTypeOrReference` = 48 AND `ConditionValue1` = 391940;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 14328 AND `SourceEntry` = 10527;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 13834, 10527, 0, 0, 48, 0, 391940, 0, 1, 0, 0, 0, '', 'Phase 13834 in area 10527: Hrun the Exiled defeated'),
(26, 14328, 10527, 0, 0, 47, 0, 55639, 8, 0, 0, 0, 0, '', 'Phase 14328 in area 10527: quest 55639 in progress'),
(26, 14328, 10527, 0, 0, 48, 0, 391940, 0, 1, 1, 0, 0, '', 'Phase 14328 in area 10527: Hrun the Exiled not defeated yet');
DELETE FROM `phase_area` WHERE `PhaseId` = 14328 AND `AreaId` = 10527;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(10527, 14328, 'Exile\'s Reach - captive Ralia until Hrun is defeated');

-- Class quests of the island had no quest_template_addon row, so any class could take them (e.g. a mage got "A Druid's Form" from Ralia).
-- AllowableClasses: warrior 1, paladin 2, hunter 4, rogue 8, priest 16, shaman 64, mage 128, warlock 256, monk 512, druid 1024
INSERT INTO `quest_template_addon` (`ID`, `AllowableClasses`) VALUES
(58914, 1),    -- A Warrior's End
(58915, 1),    -- Hjalmar's Final Execution
(59971, 1),    -- A Warrior's End (Horde)
(59972, 1),    -- Hjalmar's Final Execution (Horde)
(58923, 2),    -- A Paladin's Service
(58946, 2),    -- The Divine's Shield
(59958, 2),    -- A Paladin's Service (Horde)
(60174, 2),    -- The Divine's Shield (Horde)
(59355, 4),    -- A Hunter's Trap
(59356, 4),    -- Hunting the Stalker
(60162, 4),    -- The Art of Taming
(59952, 4),    -- A Hunter's Trap (Horde)
(59953, 4),    -- Hunting the Stalker (Horde)
(60168, 4),    -- The Art of Taming (Horde)
(58917, 8),    -- A Rogue's End
(58933, 8),    -- The Deadliest of Poisons
(59967, 8),    -- A Rogue's End (Horde)
(59968, 8),    -- The Deadliest of Poisons (Horde)
(58953, 16),   -- A Priest's End
(58960, 16),   -- Resurrecting the Recruits
(59961, 16),   -- A Priest's End (Horde)
(59965, 16),   -- Resurrecting the Recruits (Horde)
(59002, 64),   -- A Shaman's Duty
(59969, 64),   -- A Shaman's Duty (Horde)
(59954, 128),  -- A Mage's Knowledge (Horde)
(59955, 128),  -- The Best Way to Use Sheep (Horde)
(58962, 256),  -- A Warlock's Bargain
(59970, 256),  -- A Warlock's Bargain (Horde)
(59347, 512),  -- A Monk's Focus
(59349, 512),  -- One Last Spar
(59956, 512),  -- A Monk's Focus (Horde)
(59957, 512),  -- One Last Spar (Horde)
(59350, 1024), -- A Druid's Form
(59951, 1024)  -- A Druid's Form (Horde)
ON DUPLICATE KEY UPDATE `AllowableClasses` = VALUES(`AllowableClasses`);

-- 54933 Freeing the Light: each necrotic energy source (gameobject) and the invisible bunny next to it are tracked by the
-- quest objective of that source; the bunny shows the beam to the Lightspawn (spell visual 94584) until the source is
-- dispersed, then 94587. Values from the retail sniff (12.0.1.66709). The spawn tracking templates of the sources were already
-- in TDB (phase 0). Retail tracks each bunny together with its source, but TC requires every member of a spawn tracking to
-- share the phase of its template, and the bunnies are in phase 13878 (gone once 54933 is rewarded, as in the sniff): they get
-- their own server-side trackings 217500001-217500004 for the same objectives.
DELETE FROM `spawn_tracking_template` WHERE `SpawnTrackingId` BETWEEN 217500001 AND 217500004;
INSERT INTO `spawn_tracking_template` (`SpawnTrackingId`, `MapId`, `PhaseId`, `PhaseGroup`, `PhaseUseFlags`, `VerifiedBuild`) VALUES
(217500001, 2175, 13878, 0, 0, 0),
(217500002, 2175, 13878, 0, 0, 0),
(217500003, 2175, 13878, 0, 0, 0),
(217500004, 2175, 13878, 0, 0, 0);
DELETE FROM `spawn_tracking_quest_objective` WHERE `SpawnTrackingId` BETWEEN 217500001 AND 217500004;
INSERT INTO `spawn_tracking_quest_objective` (`SpawnTrackingId`, `QuestObjectiveId`, `VerifiedBuild`) VALUES
(217500001, 390022, 0),
(217500002, 390023, 0),
(217500003, 390024, 0),
(217500004, 390025, 0);
DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN (1794179, 1794181, 1794182, 1794183) OR `SpawnTrackingId` BETWEEN 217500001 AND 217500004;
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(217500001, 0, 1051410, '390022'), -- InvisBunny 155572, first source
(1794179, 1, 600230, '390022'),    -- source 326716
(217500002, 0, 1051411, '390023'), -- InvisBunny 164022, second source
(1794181, 1, 600232, '390023'),    -- source 326717
(217500003, 0, 1051412, '390024'), -- InvisBunny 164025, third source
(1794182, 1, 600233, '390024'),    -- source 326718
(217500004, 0, 1051408, '390025'), -- InvisBunny 164026, fourth source
(1794183, 1, 600231, '390025');    -- source 326719
DELETE FROM `spawn_tracking_state` WHERE (`SpawnType` = 0 AND `SpawnId` IN (1051408, 1051410, 1051411, 1051412)) OR (`SpawnType` = 1 AND `SpawnId` IN (600230, 600231, 600232, 600233));
INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`, `StateSpellVisualId`, `StateAnimId`, `StateAnimKitId`, `StateWorldEffects`) VALUES
(0, 1051410, 0, 1, 94584, 1828, NULL, ''), (0, 1051410, 1, 1, 94584, 1828, NULL, ''), (0, 1051410, 2, 1, 94587, 1828, NULL, ''),
(0, 1051411, 0, 1, 94584, 1828, NULL, ''), (0, 1051411, 1, 1, 94584, 1828, NULL, ''), (0, 1051411, 2, 1, 94587, 1828, NULL, ''),
(0, 1051412, 0, 1, 94584, 1828, NULL, ''), (0, 1051412, 1, 1, 94584, 1828, NULL, ''), (0, 1051412, 2, 1, 94587, 1828, NULL, ''),
(0, 1051408, 0, 1, 94584, 1828, NULL, ''), (0, 1051408, 1, 1, 94584, 1828, NULL, ''), (0, 1051408, 2, 1, 94587, 1828, NULL, ''),
(1, 600230, 0, 1, NULL, 1828, NULL, ''), (1, 600230, 1, 1, NULL, 1828, NULL, ''), (1, 600230, 2, 1, NULL, 1828, NULL, ''),
(1, 600232, 0, 1, NULL, 1828, NULL, ''), (1, 600232, 1, 1, NULL, 1828, NULL, ''), (1, 600232, 2, 1, NULL, 1828, NULL, ''),
(1, 600233, 0, 1, NULL, 1828, NULL, ''), (1, 600233, 1, 1, NULL, 1828, NULL, ''), (1, 600233, 2, 1, NULL, 1828, NULL, ''),
(1, 600231, 0, 1, NULL, 1828, NULL, ''), (1, 600231, 1, 1, NULL, 1828, NULL, ''), (1, 600231, 2, 1, NULL, 1828, NULL, '');

-- 55763 The Rescue of Meredy Huntswell: the captive Meredy (phase 13566) is shown until the quest is complete. Then the
-- quest script summons the freed Meredy in her place, who breaks out (313541), says her line and runs off, as in retail.
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_meredy_captive' WHERE `entry` = 153211;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 13566 AND `SourceEntry` IN (10528, 10588);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 13566, 10528, 0, 0, 6, 0, 469, 0, 0, 0, 0, 0, '', 'Phase 13566 in area 10528: player is Alliance'),
(26, 13566, 10528, 0, 0, 47, 0, 55763, 9, 0, 0, 0, 0, '', 'Phase 13566 in area 10528: quest 55763 not taken or in progress'),
(26, 13566, 10588, 0, 0, 6, 0, 469, 0, 0, 0, 0, 0, '', 'Phase 13566 in area 10588: player is Alliance'),
(26, 13566, 10588, 0, 0, 47, 0, 55763, 9, 0, 0, 0, 0, '', 'Phase 13566 in area 10588: quest 55763 not taken or in progress');
DELETE FROM `phase_area` WHERE `PhaseId` = 13566 AND `AreaId` IN (10528, 10588);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(10528, 13566, 'Exile\'s Reach - captive Meredy until 55763 is complete'),
(10588, 13566, 'Exile\'s Reach - captive Meredy until 55763 is complete');

-- 55981 Right Beneath Their Eyes: "Enter Darkmaul Citadel", "Find the ogre cooking area" and "Reach the citadel entrance" are
-- quest objectives of type 19 (enter areatrigger 16996, 17000, 16893), and the /point, /wave and /dance objectives need the
-- auras 299483 (Around Front Door) or 299486 (Around Cooking Area). These are server-side areatriggers that TDB lacks.
-- Retail sends no data for them, so they are placed from the sniffed player: where each credit came, and where each aura was
-- applied and removed (front door aura from 407,-2082 to 451,-2047; cooking area aura and its credit from 563,-2062 to 618,-2073).
DELETE FROM `areatrigger_template` WHERE (`Id` IN (16893, 16996, 17000) AND `IsCustom` = 0) OR (`Id` = 217500001 AND `IsCustom` = 1);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `ActionSetId`, `ActionSetFlags`, `VerifiedBuild`) VALUES
(16893, 0, 1, 0, 0, 0),
(16996, 0, 1, 0, 0, 0),
(17000, 0, 1, 0, 0, 0),
(217500001, 1, 1, 0, 0, 0);
DELETE FROM `areatrigger_template_actions` WHERE (`AreaTriggerId` = 17000 AND `IsCustom` = 0) OR (`AreaTriggerId` = 217500001 AND `IsCustom` = 1);
INSERT INTO `areatrigger_template_actions` (`AreaTriggerId`, `IsCustom`, `ActionType`, `ActionParam`, `TargetType`) VALUES
(17000, 0, 0, 299486, 0),
(217500001, 1, 0, 299483, 0);
DELETE FROM `areatrigger_create_properties` WHERE `Id` BETWEEN 217500001 AND 217500004 AND `IsCustom` = 1;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Shape`, `ShapeData0`, `ShapeData1`, `VerifiedBuild`) VALUES
(217500001, 1, 16996, 0, 0, 10, 10, 0),
(217500002, 1, 17000, 0, 0, 28, 28, 0),
(217500003, 1, 16893, 0, 0, 10, 10, 0),
(217500004, 1, 217500001, 1, 0, 28, 28, 0);
DELETE FROM `areatrigger` WHERE `SpawnId` BETWEEN 217500001 AND 217500004;
INSERT INTO `areatrigger` (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES
(217500001, 217500001, 1, 2175, '0', 423.18, -2068.01, 128.23, 0, 0, 0, 0, '', 'Exile\'s Reach - 55981 Enter Darkmaul Citadel', 0),
(217500002, 217500002, 1, 2175, '0', 590.38, -2067.82, 158.8, 0, 0, 0, 0, '', 'Exile\'s Reach - 55981 Find the ogre cooking area, aura Around Cooking Area', 0),
(217500003, 217500003, 1, 2175, '0', 688.6, -1914.6, 183.19, 0, 0, 0, 0, '', 'Exile\'s Reach - 55981 Reach the citadel entrance', 0),
(217500004, 217500004, 1, 2175, '0', 429.5, -2064.6, 133.38, 0, 0, 0, 0, '', 'Exile\'s Reach - 55981 aura Around Front Door', 0);

-- 55981 Right Beneath Their Eyes: when the player dances in the cooking area, retail replaces Grunk, Wug and Jugnug by
-- personal copies that dance along for about 10 seconds and then walk off
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_dancing_ogre' WHERE `entry` IN (154145, 154146, 154147);

-- 55990 Controlling their Stones: the captive Kelra (156954) is shown until the quest is complete; then the quest script lets
-- her personal copy thank the player and phases in the freed Kelra (156965, phase 13843), who takes the quest. Before, the
-- freed Kelra was in the assault camp phase 13839 and stood inside the captive one.
-- Retail keeps the captive in phase 13334, which is not cosmetic: in TC a non-cosmetic phase hides every unphased object
-- (catapults, citadel ogres), so the captive uses the cosmetic phase 13566 here (its other area rule is the harpy nest).
UPDATE `creature_template` SET `ScriptName` = 'npc_exiles_reach_kelra_captive' WHERE `entry` = 156954;
UPDATE `creature` SET `PhaseId` = 13843 WHERE `guid` = 90010008 AND `id` = 156965;
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
(55990, 'q55990_controlling_their_stones')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (13334, 13566) AND `SourceEntry` = 10530;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 13566, 10530, 0, 0, 6, 0, 469, 0, 0, 0, 0, 0, '', 'Phase 13566 in area 10530: player is Alliance'),
(26, 13566, 10530, 0, 0, 47, 0, 55990, 9, 0, 0, 0, 0, '', 'Phase 13566 in area 10530: quest 55990 not taken or in progress');
DELETE FROM `phase_area` WHERE `PhaseId` IN (13334, 13566) AND `AreaId` = 10530;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(10530, 13566, 'Exile\'s Reach - captive Kelra until 55990 is complete');

-- 55989 Catapult Destruction: each catapult is tracked by the quest objective. Retail changes the state visual of a destroyed
-- catapult over time for that player: 100772 (bomb with a timer, 2 s), 100773 (explosion, 1 s), then 86956 (burning wreck),
-- with world effects 2099 and 2654. The spawn tracking state shows the burning wreck; go_exiles_reach_catapult shows the
-- bomb and the explosion before it.
-- Tracking ids and visuals from the retail sniff (12.0.1.66709), which sends them in phase 15275; the templates use phase 0
-- like the catapults here.
DELETE FROM `spawn_tracking_template` WHERE `SpawnTrackingId` IN (1760507, 1760509, 1760523, 1760524);
INSERT INTO `spawn_tracking_template` (`SpawnTrackingId`, `MapId`, `PhaseId`, `PhaseGroup`, `PhaseUseFlags`, `VerifiedBuild`) VALUES
(1760507, 2175, 0, 0, 0, 66709),
(1760509, 2175, 0, 0, 0, 66709),
(1760523, 2175, 0, 0, 0, 66709),
(1760524, 2175, 0, 0, 0, 66709);
DELETE FROM `spawn_tracking_quest_objective` WHERE `SpawnTrackingId` IN (1760507, 1760509, 1760523, 1760524);
INSERT INTO `spawn_tracking_quest_objective` (`SpawnTrackingId`, `QuestObjectiveId`, `VerifiedBuild`) VALUES
(1760507, 389991, 66709),
(1760509, 389991, 66709),
(1760523, 389991, 66709),
(1760524, 389991, 66709);
DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN (1760507, 1760509, 1760523, 1760524);
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(1760524, 1, 90010001, '389991'),
(1760507, 1, 90010002, '389991'),
(1760523, 1, 90010003, '389991'),
(1760509, 1, 90010004, '389991');
DELETE FROM `spawn_tracking_state` WHERE `SpawnType` = 1 AND `SpawnId` IN (90010001, 90010002, 90010003, 90010004);
INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`, `StateSpellVisualId`, `StateAnimId`, `StateAnimKitId`, `StateWorldEffects`) VALUES
(1, 90010001, 0, 1, NULL, 1828, NULL, ''), (1, 90010001, 1, 1, NULL, 1828, NULL, ''), (1, 90010001, 2, 1, 86956, 1828, NULL, '2099,2654'),
(1, 90010002, 0, 1, NULL, 1828, NULL, ''), (1, 90010002, 1, 1, NULL, 1828, NULL, ''), (1, 90010002, 2, 1, 86956, 1828, NULL, '2099,2654'),
(1, 90010003, 0, 1, NULL, 1828, NULL, ''), (1, 90010003, 1, 1, NULL, 1828, NULL, ''), (1, 90010003, 2, 1, 86956, 1828, NULL, '2099,2654'),
(1, 90010004, 0, 1, NULL, 1828, NULL, ''), (1, 90010004, 1, 1, NULL, 1828, NULL, ''), (1, 90010004, 2, 1, 86956, 1828, NULL, '2099,2654');

-- 55990 Controlling their Stones: Grunk, Wug and Jugnug drop the ward stones. TDB respawned them after 2 hours, so a player who
-- had killed them before taking the quest found none; in the retail sniff they are back 60 seconds after their death.
UPDATE `creature` SET `spawntimesecs` = 60 WHERE `guid` IN (1051414, 1051430, 1051440) AND `id` IN (153581, 153582, 153583);

-- Catapult Destruction exists twice on the same four catapults: 326651 for the Alliance (55989) and 351477 for the Horde
-- (59980). Both are always spawned, so each player saw two catapults in one another. The Alliance sniff never sees 351477.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 8 AND `SourceEntry` IN (326651, 351477);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(32, 8, 326651, 0, 0, 6, 0, 469, 0, 0, 0, 0, 0, '', 'Catapult (55989) is visible to the Alliance only'),
(32, 8, 351477, 0, 0, 6, 0, 67, 0, 0, 0, 0, 0, '', 'Catapult (59980) is visible to the Horde only');

-- 55989 Catapult Destruction: bomb and explosion before the burning wreck (see the spawn tracking states above)
UPDATE `gameobject_template` SET `ScriptName` = 'go_exiles_reach_catapult' WHERE `entry` = 326651;

-- 55990 Controlling their Stones: each ogre runestone is tracked together with the bunny (161306) next to it, which channels the
-- Necrotic Ritual (314144) on the captive Kelra. Once a runestone is disabled retail removes its bunny for that player, so the
-- beam goes out, and the runestone can no longer be used. Tracking ids from the retail sniff (12.0.1.66709); retail keeps them
-- in phase 14847, here everything is in phase 0 (TC requires the spawns to share the phase of their tracking).
-- Two runestones were stored in their used state (0); retail sends them ready (1).
UPDATE `gameobject` SET `state` = 1, `PhaseId` = 0 WHERE `guid` IN (90010005, 90010006, 90010007) AND `id` = 339865;
DELETE FROM `spawn_tracking_template` WHERE `SpawnTrackingId` IN (1868473, 1868522, 1868523);
INSERT INTO `spawn_tracking_template` (`SpawnTrackingId`, `MapId`, `PhaseId`, `PhaseGroup`, `PhaseUseFlags`, `VerifiedBuild`) VALUES
(1868473, 2175, 0, 0, 0, 66709),
(1868522, 2175, 0, 0, 0, 66709),
(1868523, 2175, 0, 0, 0, 66709);
DELETE FROM `spawn_tracking_quest_objective` WHERE `SpawnTrackingId` IN (1868473, 1868522, 1868523);
INSERT INTO `spawn_tracking_quest_objective` (`SpawnTrackingId`, `QuestObjectiveId`, `VerifiedBuild`) VALUES
(1868473, 394292, 66709),
(1868522, 394292, 66709),
(1868523, 394292, 66709);
DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN (1868473, 1868522, 1868523);
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(1868473, 1, 90010005, '394292'),     -- runestone at 713,-1873
(1868473, 0, 217500000064, '394292'), -- its bunny
(1868522, 1, 90010006, '394292'),     -- runestone at 702,-1869
(1868522, 0, 217500000065, '394292'), -- its bunny
(1868523, 1, 90010007, '394292'),     -- runestone at 711,-1862
(1868523, 0, 217500000066, '394292'); -- its bunny
DELETE FROM `spawn_tracking_state` WHERE (`SpawnType` = 1 AND `SpawnId` IN (90010005, 90010006, 90010007)) OR (`SpawnType` = 0 AND `SpawnId` IN (217500000064, 217500000065, 217500000066));
INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`, `StateSpellVisualId`, `StateAnimId`, `StateAnimKitId`, `StateWorldEffects`) VALUES
(1, 90010005, 0, 1, NULL, 1828, NULL, ''), (1, 90010005, 1, 1, NULL, 1828, NULL, '2099'), (1, 90010005, 2, 1, NULL, 1828, NULL, ''),
(1, 90010006, 0, 1, NULL, 1828, NULL, ''), (1, 90010006, 1, 1, NULL, 1828, NULL, '2099'), (1, 90010006, 2, 1, NULL, 1828, NULL, ''),
(1, 90010007, 0, 1, NULL, 1828, NULL, ''), (1, 90010007, 1, 1, NULL, 1828, NULL, '2099'), (1, 90010007, 2, 1, NULL, 1828, NULL, ''),
(0, 217500000064, 0, 1, NULL, 1828, NULL, ''), (0, 217500000064, 1, 1, NULL, 1828, NULL, ''), (0, 217500000064, 2, 0, NULL, 1828, NULL, ''),
(0, 217500000065, 0, 1, NULL, 1828, NULL, ''), (0, 217500000065, 1, 1, NULL, 1828, NULL, ''), (0, 217500000065, 2, 0, NULL, 1828, NULL, ''),
(0, 217500000066, 0, 1, NULL, 1828, NULL, ''), (0, 217500000066, 1, 1, NULL, 1828, NULL, ''), (0, 217500000066, 2, 0, NULL, 1828, NULL, '');
-- the bunnies had no script, so there was no beam at all: they channel 314144 on the captive Kelra as in the sniff
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 161306 AND `ScriptName` = '';
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (-217500000064, -217500000065, -217500000066) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(-217500000064, 0, 0, 0, 1, 0, 100, 0, 1000, 1000, 0, 0, 0, 11, 314144, 2, 0, 0, 0, 0, 19, 156954, 30, 0, 0, 0, 0, 0, 'Invisible Stalker - Out of Combat - Cast Necrotic Ritual on Captain Kelra'),
(-217500000065, 0, 0, 0, 1, 0, 100, 0, 1000, 1000, 0, 0, 0, 11, 314144, 2, 0, 0, 0, 0, 19, 156954, 30, 0, 0, 0, 0, 0, 'Invisible Stalker - Out of Combat - Cast Necrotic Ritual on Captain Kelra'),
(-217500000066, 0, 0, 0, 1, 0, 100, 0, 1000, 1000, 0, 0, 0, 11, 314144, 2, 0, 0, 0, 0, 19, 156954, 30, 0, 0, 0, 0, 0, 'Invisible Stalker - Out of Combat - Cast Necrotic Ritual on Captain Kelra');
