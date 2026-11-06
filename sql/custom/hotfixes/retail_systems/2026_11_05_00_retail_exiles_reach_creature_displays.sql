-- creature displays of Exile's Reach / Darkmaul Citadel creatures (Ravnyr 156501: 114576) that the server DB2 files
-- (extracted from an older client) do not have; without them the creature has no usable model and is not spawned.
-- Rows from CreatureDisplayInfo of 12.1.5.70783.
DELETE FROM `creature_display_info` WHERE `ID` IN (114576, 138759, 139992);
INSERT INTO `creature_display_info` (`ID`,`ModelID`,`SoundID`,`SizeClass`,`CreatureModelScale`,`CreatureModelAlpha`,`BloodID`,`ExtendedDisplayInfoID`,`NPCSoundID`,`ParticleColorID`,`PortraitCreatureDisplayInfoID`,`PortraitTextureFileDataID`,`ObjectEffectPackageID`,`AnimReplacementSetID`,`Flags`,`StateSpellVisualKitID`,`PlayerOverrideScale`,`PetInstanceScale`,`UnarmedWeaponType`,`MountPoofSpellVisualKitID`,`DissolveEffectID`,`Gender`,`DissolveOutEffectID`,`CreatureModelMinLod`,`ConditionalCreatureModelID`,`Unknown_1100_1`,`Unknown_1100_2`,`TextureVariationFileDataID1`,`TextureVariationFileDataID2`,`TextureVariationFileDataID3`,`TextureVariationFileDataID4`,`VerifiedBuild`) VALUES
(114576, 13277, 7899, 4, 2, 255, 0, 0, 2681, 0, 0, 0, 0, 0, 8, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 45, 0, 4496931, 0, 0, 0, 70783),
(138759, 7050, 4749, -1, 1, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 2, 0, 0, 0, 45, 0, 929443, 929444, 0, 0, 70783),
(139992, 7062, 10024, 0, 1, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.25, 1, -1, 0, 0, 0, 0, 0, 0, 45, 0, 937286, 960032, 960033, 0, 70783);
