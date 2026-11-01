-- Exile's Reach: SummonProperties used by the island's summon spells. Retail 12.x removed these rows from the client
-- data (sniff 12.0.1.66709 sends them as removed hotfixes), so the core aborts the summons (no Scout-o-Matic to ride,
-- no sparring partner, prisoners, guardians). Values from SummonProperties.db2 of build 11.0.2.56421; server-side only.
DELETE FROM `summon_properties` WHERE `VerifiedBuild` = 0 AND `ID` IN (4474, 4627, 4693, 4694, 4695, 4698, 4835, 4845, 4846, 4857, 4858, 4887, 4889, 5056, 5060, 5063);
INSERT INTO `summon_properties` (`ID`, `Control`, `Faction`, `Title`, `Slot`, `Flags1`, `Flags2`, `VerifiedBuild`) VALUES
(4474, 0, 0, 0, 0, 67242008, 0, 0),   -- 305742: 156736
(4627, 1, 0, 0, 0, 8520464, 3, 0),    -- 298235: Henry Garrick (ogre camp prisoner)
(4693, 4, 0, 0, 0, 4120, 0, 0),       -- 305523: Scout-o-Matic 5000 (vehicle), 326624: Choppy Booster Mk. 5
(4694, 4, 0, 0, 0, 4120, 0, 0),       -- 305069: 156267
(4695, 1, 0, 0, 0, 8519936, 3, 0),    -- 305779: 174955
(4698, 1, 0, 0, 0, 8520464, 3, 0),    -- 306145: Jaina Proudmoore (ogre camp prisoner)
(4835, 0, 0, 0, 0, 1082130720, 0, 0), -- 320175: Alliance Sparring Partner
(4845, 0, 0, 0, 0, 1082130720, 0, 0), -- 321148: Private Cole (polymorph training)
(4846, 0, 0, 0, 0, 1082130720, 0, 0), -- 321149: Meredy Huntswell (polymorph training)
(4857, 1, 0, 0, 0, 8519936, 3, 0),    -- 305596: Alliance Survivor
(4858, 1, 0, 0, 0, 8519936, 3, 0),    -- 305661: Horde survivor
(4887, 0, 0, 0, 0, 1082130720, 0, 0), -- 325108: 166814
(4889, 0, 0, 0, 0, 1082130720, 0, 0), -- 325181: Horde Sparring Partner
(5056, 1, 0, 0, 0, 8519952, 3, 0),    -- 326635: 167915
(5060, 1, 0, 0, 0, 8519952, 3, 0),    -- 305749: Lindie Springstock (guardian)
(5063, 1, 0, 0, 0, 8519952, 3, 0);    -- 344383: 175034
