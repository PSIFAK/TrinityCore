-- model info of the creature displays added in hotfixes 2026_11_05_00_retail_exiles_reach_creature_displays.sql; without a row
-- the creature still has no usable model and is not spawned (Ravnyr 156501). Values from the retail 12.1.5 sniff, divided by
-- the display scale the sniff sent them with (139992 has no sniff data, it uses the one of 139991 that retail shows instead).
DELETE FROM `creature_model_info` WHERE `DisplayID` IN (114576, 138759, 139992);
INSERT INTO `creature_model_info` (`DisplayID`, `BoundingRadius`, `CombatReach`, `DisplayID_Other_Gender`, `VerifiedBuild`) VALUES
(114576, 19.522968, 10, 0, 70688),
(138759, 21.045852, 13.469388, 0, 70688),
(139992, 0.875057, 1.53, 0, 70688);
