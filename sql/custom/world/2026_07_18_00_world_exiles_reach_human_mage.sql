-- Human Mage Exile's Reach continuation captured on 12.0.1.66709.
-- Character: Human Mage, level 1; map flow 2175 -> 2236 -> 2175 -> 2444.

SET @CGUID := 90010000;
SET @OGUID := 90010000;

-- Quest chain and class restriction.
INSERT INTO `quest_template_addon` (`ID`, `AllowableClasses`, `PrevQuestID`, `ScriptName`) VALUES
(55639, 0,   55965, ''),
(85678, 0,   55639, ''),
(55196, 0,   85678, ''),
(54933, 0,   55196, 'q54933_lights_camera_action'),
(55763, 0,   54933, ''),
(55881, 0,   54933, ''),
(55764, 0,   54933, ''),
(55882, 0,   55764, ''),
(59352, 128, 55882, ''),
(59354, 128, 59352, ''),
(56344, 0,   59354, ''),
(56839, 0,  -56344, ''),
(55981, 0,   56344, ''),
(55990, 0,   55981, ''),
(55988, 0,   55981, ''),
(55989, 0,   55981, ''),
(55992, 0,   55989, ''),
(55991, 0,   55992, ''),
(87547, 0,   55991, ''),
(87555, 0,   87547, '')
ON DUPLICATE KEY UPDATE
`AllowableClasses` = VALUES(`AllowableClasses`),
`PrevQuestID` = VALUES(`PrevQuestID`),
`ScriptName` = VALUES(`ScriptName`);

-- Repair the existing first-level portion of the chain without replacing its scripted scenes.
INSERT INTO `quest_template_addon` (`ID`, `AllowableClasses`, `PrevQuestID`) VALUES
(56775, 0,    0),
(58209, 0,    56775),
(58208, 0,    58209),
(55122, 0,    58208),
(54951, 0,    55122),
(54952, 0,    54951),
(55174, 0,    54952),
(59254, 1499, 55174),
(55173, 0,    59254),
(55186, 0,    55173),
(55184, 0,    55173),
(55193, 0,    55184),
(56034, 0,    55193),
(55879, 0,    56034),
(55194, 0,    55879),
(55965, 0,    55194)
ON DUPLICATE KEY UPDATE `PrevQuestID` = VALUES(`PrevQuestID`);

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` IN (55173, 55193);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(19, 0, 55173, 0, 0, 47, 0, 55174, 64, 0, '', 0, 0, 0, '', 'Northbound requires Cooking Meat rewarded'),
(19, 0, 55173, 0, 0, 47, 0, 59254, 64, 0, '', 0, 0, 0, '', 'Northbound requires Enhanced Combat Tactics rewarded'),
(19, 0, 55193, 0, 0, 47, 0, 55186, 64, 0, '', 0, 0, 0, '', 'Scout-o-Matic 5000 requires Down with the Quilboar rewarded'),
(19, 0, 55193, 0, 0, 47, 0, 55184, 64, 0, '', 0, 0, 0, '', 'Scout-o-Matic 5000 requires Forbidden Quilboar Necromancy rewarded');

-- Parallel branches must all be completed before the convergence quest appears.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` IN (55882, 55992);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(19, 0, 55882, 0, 0, 47, 0, 55763, 64, 0, '', 0, 0, 0, '', 'Message to Base requires Rescuing Meredy Huntswell rewarded'),
(19, 0, 55882, 0, 0, 47, 0, 55881, 64, 0, '', 0, 0, 0, '', 'Message to Base requires Purge the Totems rewarded'),
(19, 0, 55882, 0, 0, 47, 0, 55764, 64, 0, '', 0, 0, 0, '', 'Message to Base requires Harpy Culling rewarded'),
(19, 0, 55992, 0, 0, 47, 0, 55990, 64, 0, '', 0, 0, 0, '', 'Darkmaul Citadel requires Controlling their Stones rewarded'),
(19, 0, 55992, 0, 0, 47, 0, 55988, 64, 0, '', 0, 0, 0, '', 'Darkmaul Citadel requires Like Ogres to the Slaughter rewarded'),
(19, 0, 55992, 0, 0, 47, 0, 55989, 64, 0, '', 0, 0, 0, '', 'Darkmaul Citadel requires Catapult Destruction rewarded');

-- Retail quest giver / turn-in identities from the sniff.
DELETE FROM `creature_queststarter`
WHERE `quest` IN (55194, 55639, 85678, 55196, 54933, 55763, 55881, 55764, 55882, 59352, 59354, 56344, 55981, 55990, 55988, 55989, 55992, 55991, 87547, 87555);
INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES
(245394, 55194),
(156803, 55639),
(245394, 85678),
(156833, 55196),
(157114, 54933),
(156859, 55763),
(156859, 55881),
(156860, 55764),
(156859, 55882),
(156886, 59352),
(156886, 59354),
(245394, 56344),
(245397, 55981),
(156961, 55990),
(156942, 55988),
(245667, 55989),
(156965, 55992),
(245686, 55991),
(238913, 87547),
(238913, 87555);

DELETE FROM `creature_questender`
WHERE `quest` IN (55194, 55639, 85678, 55196, 54933, 55763, 55881, 55764, 55882, 59352, 59354, 56344, 56839, 55981, 55990, 55988, 55989, 55992, 55991, 87547);
INSERT INTO `creature_questender` (`id`, `quest`) VALUES
(245394, 55194),
(245394, 55639),
(245394, 85678),
(156859, 55196),
(157114, 54933),
(156882, 55763),
(156859, 55881),
(156860, 55764),
(245394, 55882),
(156886, 59352),
(156886, 59354),
(245397, 56344),
(155733, 56839),
(156961, 55981),
(156965, 55990),
(156942, 55988),
(245667, 55989),
(156961, 55992),
(245686, 55992),
(238913, 55991),
(238913, 87547);

DELETE FROM `gameobject_queststarter` WHERE `quest` = 56839;
INSERT INTO `gameobject_queststarter` (`id`, `quest`) VALUES (330627, 56839);

-- Quest giver and script flags.
UPDATE `creature_template`
SET `npcflag` = `npcflag` | 2
WHERE `entry` IN (156803, 245394, 156833, 157114, 156859, 156860, 156882, 156886, 245397, 155733, 156961, 156942, 245667, 156965, 245686, 238913);

UPDATE `creature_template` SET `faction` = 35 WHERE `entry` IN
(156800, 156803, 245394, 156833, 157114, 156859, 156860, 156882, 156886, 156929, 245397, 155733, 156961, 156942, 245667, 156965, 161350, 244389, 245686, 238913, 187412);

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_ralia_dreamchaser', `npcflag` = `npcflag` | 16777216 WHERE `entry` = 156929;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_quartermaster_richter' WHERE `entry` = 156800;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_meredy_polymorph_training' WHERE `entry` = 156886;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_meredy_ogre_transformation' WHERE `entry` = 156943;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = '', `npcflag` = `npcflag` & ~1 WHERE `entry` = 156965;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_kalecgos_travel', `npcflag` = `npcflag` | 1 WHERE `entry` = 244389;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_exiles_reach_happy_hal' WHERE `entry` = 187412;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_gorgroth' WHERE `entry` = 156814;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_ravnyr' WHERE `entry` = 156501;

-- Ralia becomes spell-clickable only after Hrun has been defeated.
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 156929;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(156929, 312463, 3, 0);

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 156929 AND `SourceEntry` = 312463;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(18, 156929, 312463, 0, 0, 47, 0, 55639, 8, 0, '', 0, 0, 0, '', 'Ralia is clickable while Who Lurks in the Pit is incomplete'),
(18, 156929, 312463, 0, 0, 48, 0, 391940, 0, 1, '', 0, 0, 0, '', 'Ralia is clickable after Hrun is defeated');

UPDATE `scene_template` SET `ScriptName` = 'scene_exiles_reach_ralia_rescue' WHERE `SceneId` = 2379;

DELETE FROM `spell_script_names` WHERE `spell_id` = 118 AND `ScriptName` = 'spell_exiles_reach_polymorph_training';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(118, 'spell_exiles_reach_polymorph_training');

-- Retail gossip menus.
INSERT IGNORE INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(39219, 0, 66709),
(39497, 0, 66709);
DELETE FROM `gossip_menu` WHERE `MenuID` = 39498;

DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (24550, 25321, 39219, 39497, 39498);
INSERT INTO `gossip_menu_option`
(`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(24550, 50819, 0, 0, 'Я готова. Преврати меня в огра, и я проникну в цитадель.', 0, 0, 1, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 66709),
(25321, 51802, 0, 0, 'Я готова научиться "Превращению".', 0, 0, 1, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 66709),
(39219, 133763, 0, 0, 'Помоги мне добраться до Драконьих островов и расскажи, что там происходит.', 0, 0, 1, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 66709),
(39497, 134083, 0, 0, 'Я готова покинуть цитадель.', 0, 0, 1, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 66709),
(39497, 134085, 1, 0, 'Расскажи, что значит быть Аспектом синих драконов.', 0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 66709);

DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (156501, 156965, 244389);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(156501, 39497, 66709),
(244389, 39219, 66709);

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` IN (24550, 25321, 39219, 39497, 39498);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 24550, 0, 0, 0, 47, 0, 55981, 8, 0, '', 0, 0, 0, '', 'Show ogre transformation while The Enemy of My Enemy is incomplete'),
(15, 24550, 0, 0, 0, 48, 0, 390131, 0, 1, '', 1, 0, 0, '', 'Hide ogre transformation after Meredy has transformed the player'),
(15, 25321, 0, 0, 0, 47, 0, 59354, 8, 0, '', 0, 0, 0, '', 'Show Polymorph training while The Art of Taming Sheep is incomplete'),
(15, 25321, 0, 0, 0, 48, 0, 396406, 0, 1, '', 1, 0, 0, '', 'Hide Polymorph training after speaking with Meredy'),
(15, 39219, 0, 0, 0, 47, 0, 55991, 8, 0, '', 0, 0, 0, '', 'Show Dragon Isles travel while The End of the Beginning is incomplete'),
(15, 39497, 0, 0, 0, 47, 0, 55992, 8, 0, '', 0, 0, 0, '', 'Show Darkmaul exit while the dungeon quest is incomplete');

-- Story phase transitions observed in SMSG_PHASE_SHIFT_CHANGE.
DELETE FROM `phase_area`
WHERE (`PhaseId` = 13840 AND `AreaId` = 10424)
   OR (`PhaseId` IN (13834, 13839, 13840, 13843, 14391, 14695, 15490, 15612) AND `AreaId` IN (10424, 10527, 10568, 11011))
   OR (`PhaseId` = 25711 AND `AreaId` = 13722);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(10527, 13834, 'Ralia rescue state in Hrun cave'),
(10568, 13840, 'Alliance heroes at Darkmaul Bridge'),
(11011, 14695, 'Deathclaw lair state'),
(10424, 15490, 'Ogre transformation transition'),
(10424, 13839, 'Alliance assault camp at Darkmaul Citadel'),
(10424, 13843, 'Darkmaul dungeon objective complete'),
(10424, 14391, 'Darkmaul dungeon quest rewarded'),
(10424, 15612, 'Alliance camp after Darkmaul Citadel'),
(13722, 25711, 'Dragon Isles arrival state');

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (13834, 13839, 13840, 13843, 14391, 14695, 15490, 15612, 25711);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 13834, 10527, 0, 0, 47, 0, 55639, 10, 0, '', 0, 0, 0, '', 'Ralia rescue phase while quest is incomplete or complete'),
(26, 13840, 10568, 0, 0, 47, 0, 56344, 74, 0, '', 0, 0, 0, '', 'Darkmaul bridge phase during Into the Darkmaul Citadel'),
(26, 13840, 10568, 0, 0, 47, 0, 55981, 66, 0, '', 1, 0, 0, '', 'Darkmaul bridge phase before The Enemy of My Enemy is completed'),
(26, 13840, 10568, 0, 0, 48, 0, 390131, 0, 1, '', 1, 0, 0, '', 'Darkmaul bridge phase before ogre transformation credit'),
(26, 14695, 11011, 0, 0, 6, 0, 469, 0, 0, '', 0, 0, 0, '', 'Deathclaw lair phase for Alliance'),
(26, 14695, 11011, 0, 0, 47, 0, 56344, 74, 0, '', 0, 0, 0, '', 'Deathclaw lair phase during the citadel approach'),
(26, 15490, 10424, 0, 0, 47, 0, 55981, 8, 0, '', 0, 0, 0, '', 'Ogre transformation phase while quest is incomplete'),
(26, 15490, 10424, 0, 0, 48, 0, 390131, 0, 1, '', 0, 0, 0, '', 'Ogre transformation phase after talking to Meredy'),
(26, 13839, 10424, 0, 0, 47, 0, 55981, 66, 0, '', 0, 0, 0, '', 'Assault camp after The Enemy of My Enemy is completed'),
(26, 13839, 10424, 0, 0, 47, 0, 55992, 66, 0, '', 1, 0, 0, '', 'Assault camp before the dungeon quest is completed'),
(26, 13843, 10424, 0, 0, 47, 0, 55990, 66, 0, '', 0, 0, 0, '', 'Dungeon-ready phase after Controlling their Stones is completed'),
(26, 13843, 10424, 0, 0, 47, 0, 55992, 66, 0, '', 1, 0, 0, '', 'Dungeon-ready phase before the dungeon quest is completed'),
(26, 14391, 10424, 0, 0, 47, 0, 55992, 74, 0, '', 0, 0, 0, '', 'Darkmaul dungeon quest active, complete or rewarded'),
(26, 14391, 10424, 0, 0, 47, 0, 55991, 64, 0, '', 1, 0, 0, '', 'Post-dungeon phase before leaving Exiles Reach'),
(26, 15612, 10424, 0, 0, 47, 0, 55992, 66, 0, '', 0, 0, 0, '', 'Post-dungeon NPC state after Darkmaul Citadel'),
(26, 15612, 10424, 0, 0, 47, 0, 55991, 66, 0, '', 1, 0, 0, '', 'Post-dungeon NPC state before The End of the Beginning is completed'),
(26, 25711, 13722, 0, 0, 47, 0, 55991, 74, 0, '', 0, 0, 0, '', 'Dragon Isles arrival after The End of the Beginning');

-- Missing and replacement story spawns.
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID AND @CGUID + 15;
INSERT INTO `creature`
(`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`, `VerifiedBuild`) VALUES
(@CGUID + 0, 245394, 2175, 10424, 10529, '0',   0, 13835, 0, 179.7361145, -2285.6997070, 81.9317551, 0.9936105, 120, 0, 0, 66709),
(@CGUID + 1, 156929, 2175, 10424, 10527, '0',   0, 13834, 0, 74.5572891, -2134.4445801, -30.0123291, 5.2725792, 120, 0, 0, 66709),
(@CGUID + 2, 156882, 2175, 10424, 10528, '0',   0, 15356, 0, 393.2187500, -2442.9306641, 125.9134445, 2.7876916, 120, 0, 0, 66709),
(@CGUID + 3, 155733, 2175, 10424, 11011, '0',   0, 14695, 0, 280.8211975, -1986.7778320, 77.7343521, 3.9874218, 120, 0, 0, 66709),
(@CGUID + 4, 245397, 2175, 10424, 10568, '0',   0, 13840, 0, 317.2552185, -2172.2431641, 106.0651932, 0.8189396, 120, 0, 0, 66709),
(@CGUID + 5, 156942, 2175, 10424, 10530, '0',   0, 13839, 0, 706.3246460, -1882.7569580, 186.5911865, 1.2961072, 120, 0, 0, 66709),
(@CGUID + 6, 245667, 2175, 10424, 10530, '0',   0, 13839, 0, 697.8333130, -1877.5625000, 186.5911865, 0.9958113, 120, 0, 0, 66709),
(@CGUID + 7, 156961, 2175, 10424, 10530, '0',   0, 13839, 0, 703.9080200, -1876.6389160, 186.9656677, 1.1826928, 120, 0, 0, 66709),
(@CGUID + 8, 156965, 2175, 10424, 10530, '0',   0, 13839, 0, 708.6909790, -1868.7934570, 186.7059631, 4.4661570, 120, 0, 0, 66709),
(@CGUID + 9, 161350, 2236, 10581, 10581, '150', 0, 0,     0, 880.7117920, -1781.1875000, 181.3409424, 3.6419549, 7200, 0, 0, 66709),
(@CGUID + 10, 244389, 2175, 10424, 10530, '0',  0, 15612, 0, 706.9166870, -1870.1492920, 186.9656830, 1.2755961, 120, 0, 0, 66709),
(@CGUID + 11, 245686, 2175, 10424, 10530, '0',  0, 15612, 0, 712.3784790, -1859.5086670, 186.9611053, 4.2474833, 120, 0, 0, 66709),
(@CGUID + 12, 156942, 2175, 10424, 10530, '0',  0, 15612, 0, 714.3784790, -1865.1562500, 186.7536926, 4.2327418, 120, 0, 0, 66709),
(@CGUID + 13, 156965, 2175, 10424, 10530, '0',  0, 15612, 0, 707.3107910, -1861.8403320, 186.7536011, 4.4713264, 120, 0, 0, 66709),
(@CGUID + 14, 238913, 2444, 13644, 13722, '0',  0, 25711, 0, 3698.5842285, -1886.4149170, 4.4755287, 6.0383234, 120, 0, 0, 66709);

-- Phase the existing quest objects and named ogres into their retail states.
UPDATE `gameobject` SET `PhaseId` = 14695, `PhaseGroup` = 0 WHERE `id` = 330627 AND `map` = 2175;
UPDATE `creature` SET `PhaseId` = 13839, `PhaseGroup` = 0 WHERE `id` IN (153581, 153582, 153583) AND `map` = 2175;
UPDATE `creature` SET `PhaseId` = 25711, `PhaseGroup` = 0 WHERE `id` = 187412 AND `map` = 2444;

-- Mage spellbook, catapults and runestones.
INSERT INTO `gameobject_template`
(`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `size`, `Data0`, `Data1`, `Data2`, `Data8`, `Data14`, `Data20`, `Data30`, `Data31`, `ContentTuningId`, `RequiredLevel`, `VerifiedBuild`) VALUES
(346273, 3, 15749, 'Meredy\'s Spellbook', 'questinteract', 'Retrieving', 1, 1691, 0, 1, 59352, 23645, 2048, 101440, 1, 741, 1, 66709)
ON DUPLICATE KEY UPDATE
`type` = VALUES(`type`), `displayId` = VALUES(`displayId`), `name` = VALUES(`name`), `IconName` = VALUES(`IconName`),
`castBarCaption` = VALUES(`castBarCaption`), `size` = VALUES(`size`), `Data0` = VALUES(`Data0`), `Data1` = VALUES(`Data1`),
`Data2` = VALUES(`Data2`), `Data8` = VALUES(`Data8`), `Data14` = VALUES(`Data14`), `Data20` = VALUES(`Data20`),
`Data30` = VALUES(`Data30`), `Data31` = VALUES(`Data31`), `ContentTuningId` = VALUES(`ContentTuningId`),
`RequiredLevel` = VALUES(`RequiredLevel`), `VerifiedBuild` = VALUES(`VerifiedBuild`);

INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(346273, 0, 0x204004, 0, 3737)
ON DUPLICATE KEY UPDATE `faction` = VALUES(`faction`), `flags` = VALUES(`flags`), `WorldEffectID` = VALUES(`WorldEffectID`), `AIAnimKitID` = VALUES(`AIAnimKitID`);

DELETE FROM `gameobject_questitem` WHERE `GameObjectEntry` = 346273 AND `Idx` = 0;
INSERT INTO `gameobject_questitem` (`GameObjectEntry`, `Idx`, `ItemId`, `VerifiedBuild`) VALUES
(346273, 0, 175975, 66709);

DELETE FROM `gameobject_loot_template` WHERE `Entry` = 101440 AND `Item` = 175975;
INSERT INTO `gameobject_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(101440, 0, 175975, 100, 1, 1, 0, 1, 1, 'Meredy\'s Spellbook');

DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID AND @OGUID + 7;
INSERT INTO `gameobject`
(`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `VerifiedBuild`) VALUES
(@OGUID + 0, 346273, 2175, 10424, 10529, '0', 0, 13836, 0, 309.9739685, -2275.1633301, 77.2677078, 4.4311733, 0.2488818, 0.2020559, -0.7546186, 0.5725224, 120, 255, 0, 66709),
(@OGUID + 1, 326651, 2175, 10424, 10530, '0', 0, 13839, 0, 489.8697815, -2051.5729980, 143.9427032, 3.6979280, 0, 0, -0.9615602, 0.2745941, 120, 255, 1, 66709),
(@OGUID + 2, 326651, 2175, 10424, 10530, '0', 0, 13839, 0, 535.9757080, -2085.5468750, 158.3209686, 4.0296631, 0, 0, -0.9030256, 0.4295867, 120, 255, 1, 66709),
(@OGUID + 3, 326651, 2175, 10424, 10530, '0', 0, 13839, 0, 610.7326660, -2118.4167480, 158.9259644, 4.4087152, 0, 0, -0.8059244, 0.5920185, 120, 255, 1, 66709),
(@OGUID + 4, 326651, 2175, 10424, 10530, '0', 0, 13839, 0, 463.7795105, -1997.0816650, 143.7128601, 2.9605317, 0, 0, 0.9959049, 0.0904069, 120, 255, 1, 66709),
(@OGUID + 5, 339865, 2175, 10424, 10530, '0', 0, 13839, 0, 713.0451660, -1873.5208740, 186.3803253, 5.4742947, 0, 0, -0.3935089, 0.9193208, 120, 255, 0, 66709),
(@OGUID + 6, 339865, 2175, 10424, 10530, '0', 0, 13839, 0, 701.9670410, -1868.9617920, 186.5078583, 6.1296496, 0, 0, -0.0766926, 0.9970548, 120, 255, 1, 66709),
(@OGUID + 7, 339865, 2175, 10424, 10530, '0', 0, 13839, 0, 711.0989380, -1862.1944580, 186.5078583, 4.3025241, 0, 0, -0.8362074, 0.5484134, 120, 255, 0, 66709);

-- Named ogres provide both the three wardstones and their shared quest credit.
UPDATE `creature_template` SET `KillCredit1` = 161300 WHERE `entry` IN (153581, 153582, 153583);
UPDATE `creature_template_difficulty` SET `LootID` = `Entry` WHERE `Entry` IN (153581, 153582, 153583) AND `DifficultyID` IN (0, 1);

DELETE FROM `creature_loot_template` WHERE `Entry` IN (153581, 153582, 153583) AND `Item` IN (168599, 168600, 168601);
INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(153581, 0, 168601, 100, 1, 1, 0, 1, 1, 'Grunk\'s Wardstone'),
(153582, 0, 168600, 100, 1, 1, 0, 1, 1, 'Wug\'s Wardstone'),
(153583, 0, 168599, 100, 1, 1, 0, 1, 1, 'Jugnug\'s Wardstone');

-- Auras visible in the sniff.
INSERT INTO `creature_template_addon` (`entry`, `visibilityDistanceType`, `auras`) VALUES
(156961, 3, '1239042'),
(161350, 3, '222584'),
(157300, 3, '330005')
ON DUPLICATE KEY UPDATE `visibilityDistanceType` = VALUES(`visibilityDistanceType`), `auras` = VALUES(`auras`);

-- Darkmaul Citadel scenario and exact 12.0.1 instance spawn state.
DELETE FROM `scenarios` WHERE `map` = 2236 AND `difficulty` = 150;
INSERT INTO `scenarios` (`map`, `difficulty`, `scenario_A`, `scenario_H`) VALUES
(2236, 150, 1779, 1779);

DELETE FROM `lfg_dungeon_template` WHERE `dungeonId` = 2043;
INSERT INTO `lfg_dungeon_template`
(`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`, `requiredItemLevel`, `VerifiedBuild`) VALUES
(2043, 'Darkmaul Citadel', 911.1528, -1765.6423, 181.19034, 3.6743245, 0, 66709);

UPDATE `creature` SET `orientation` = 1.5808135 WHERE `id` = 156501 AND `map` = 2236;
UPDATE `creature` SET `orientation` = 5.9797382 WHERE `id` = 156814 AND `map` = 2236;
UPDATE `creature` SET `orientation` = 2.1256483 WHERE `id` = 157300 AND `map` = 2236;
