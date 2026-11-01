-- Darkmaul Citadel: the three allies at the entrance follow the player and fight as tank, healer and damage
-- (npc_darkmaul_allies.cpp, on RetailSystems' CompanionAI). In the retail sniff (12.0.1.66709) they are instance
-- spawns of faction 1665, not party members.
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_darkmaul_captain_garrick', `faction` = 1665 WHERE `entry` = 161350;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_darkmaul_henry_garrick' WHERE `entry` = 161504;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_darkmaul_jaina' WHERE `entry` = 245401;

-- weapons as in the retail sniff: Captain Garrick sword and shield (his equipment existed but the spawn did not use it),
-- Jaina her staff; Henry Garrick fights unarmed
DELETE FROM `creature_equip_template` WHERE `CreatureID` = 245401;
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, `ItemID2`, `AppearanceModID2`, `ItemVisual2`, `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES
(245401, 1, 153575, 0, 0, 0, 0, 0, 0, 0, 0, 66709);
UPDATE `creature` SET `equipment_id` = 1 WHERE `id` = 161350 AND `map` = 2236;
