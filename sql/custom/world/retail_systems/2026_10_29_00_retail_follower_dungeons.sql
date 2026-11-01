-- RetailSystems: follower dungeons (difficulty 205). The dungeon finder group gets these NPCs for the roles no player
-- took (FollowerDungeonService, follower_dungeon_npcs.cpp).
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_follower_captain_garrick', `BaseAttackTime` = 2000, `RangeAttackTime` = 2000 WHERE `entry` = 209057;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_follower_crenna_earth_daughter', `BaseAttackTime` = 2000, `RangeAttackTime` = 2000 WHERE `entry` = 209072;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_follower_meredy_huntswell', `BaseAttackTime` = 2000, `RangeAttackTime` = 2000 WHERE `entry` = 209059;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_follower_austin_huxworth', `BaseAttackTime` = 2000, `RangeAttackTime` = 2000 WHERE `entry` = 209065;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_follower_shuja_grimaxe', `BaseAttackTime` = 2000, `RangeAttackTime` = 2000 WHERE `entry` = 214390;

-- Spawns are kept per difficulty and TDB has none for difficulty 205 on most follower dungeon maps: the follower version
-- uses the spawns of the normal one (difficulty 1). Maps of every LFGDungeons.db2 row with difficulty 205 (client 12.0.7).
SET @FOLLOWER_MAPS := '2451,2515,2516,2519,2520,2521,2526,2527,2648,2649,2651,2652,2660,2661,2662,2669,2773,2805,2811,2813,2825,2830,2859,2874,2915,2923,2993';
UPDATE `creature` SET `spawnDifficulties` = CONCAT(`spawnDifficulties`, ',205')
    WHERE FIND_IN_SET(`map`, @FOLLOWER_MAPS) AND FIND_IN_SET('1', `spawnDifficulties`) AND NOT FIND_IN_SET('205', `spawnDifficulties`);
UPDATE `gameobject` SET `spawnDifficulties` = CONCAT(`spawnDifficulties`, ',205')
    WHERE FIND_IN_SET(`map`, @FOLLOWER_MAPS) AND FIND_IN_SET('1', `spawnDifficulties`) AND NOT FIND_IN_SET('205', `spawnDifficulties`);
UPDATE `areatrigger` SET `SpawnDifficulties` = CONCAT(`SpawnDifficulties`, ',205')
    WHERE FIND_IN_SET(`MapId`, @FOLLOWER_MAPS) AND FIND_IN_SET('1', `SpawnDifficulties`) AND NOT FIND_IN_SET('205', `SpawnDifficulties`);
