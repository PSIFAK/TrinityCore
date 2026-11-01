-- Darkmaul Citadel: details of the run from the retail sniff (12.0.1.66709) that the scripts need

-- Ravnyr warns the player hit by Dark Breath (RaidBossWhisper)
DELETE FROM `creature_text` WHERE `CreatureID` = 156501;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(156501, 0, 0, '|TINTERFACE\\ICONS\\Spell_Necro_Conclave.blp:20|t Avoid Kalecgos\'s |cFFFF0404|Hspell:305515|h[Dark Breath]|h|r by moving behind him!', 42, 0, 100, 0, 0, 0, 0, 0, 0, 'Ravnyr to Player');
DELETE FROM `creature_text_locale` WHERE `CreatureID` = 156501;
INSERT INTO `creature_text_locale` (`CreatureID`, `GroupID`, `ID`, `Locale`, `Text`) VALUES
(156501, 0, 0, 'ruRU', '|TINTERFACE\\ICONS\\Spell_Necro_Conclave.blp:20|t Уклоняйтесь от |cFFFF0404|Hspell:305515|h[Темного дыхания]|h|r Калесгоса, забегая ему за спину!');

-- the storm over the last hall (Necrotic Storm, 321904 when Ravnyr engages: boss_ravnyr.cpp)
DELETE FROM `creature_addon` WHERE `guid` = 1051960;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(1051960, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '321806');

-- trash abilities: Darkmaul Channeler (Mind Flay), Darkmaul Centurion (Cleave every ~10 s, Kick in the Face)
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 156825 AND `ScriptName` = '';
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND ((`entryorguid` = 157328 AND `id` = 1) OR `entryorguid` = 156825);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(157328, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 15000, 20000, 0, 11, 321330, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkmaul Channeler - In Combat - Cast Mind Flay'),
(156825, 0, 0, 0, 0, 0, 100, 0, 3000, 5000, 9000, 11000, 0, 11, 306389, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkmaul Centurion - In Combat - Cast Cleave'),
(156825, 0, 1, 0, 0, 0, 100, 0, 6000, 8000, 18000, 22000, 0, 11, 306399, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkmaul Centurion - In Combat - Cast Kick in the Face');
