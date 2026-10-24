-- Halls of Valor (map 1477) scripts, ported from lun-core (DekkCore Legion/HallsOfValor)

-- Instance script (Map.db2 ParentMapID is -1)
DELETE FROM `instance_template` WHERE `map`=1477;
INSERT INTO `instance_template` (`map`,`parent`,`script`) VALUES
(1477,0,'instance_halls_of_valor');

-- Hymdall
UPDATE `creature_template` SET `ScriptName`='boss_hymdall' WHERE `entry`=94960;
UPDATE `creature_template` SET `ScriptName`='npc_hymdall_storm_drake' WHERE `entry`=97788;

-- Hyrja
UPDATE `creature_template` SET `ScriptName`='boss_hyrja' WHERE `entry`=95833;
UPDATE `creature_template` SET `ScriptName`='npc_hyrja_defender' WHERE `entry` IN (97202,97219);

-- Fenryr
UPDATE `creature_template` SET `ScriptName`='boss_fenryr' WHERE `entry`=99868;
UPDATE `creature_template` SET `ScriptName`='npc_hov_fenryr' WHERE `entry`=95674;
UPDATE `creature_template` SET `ScriptName`='npc_fenryr_ebonclaw_packmate' WHERE `entry`=99922;

-- God-King Skovald and the Valarjar kings
UPDATE `creature_template` SET `ScriptName`='boss_god_king_skovald' WHERE `entry`=95675;
UPDATE `creature_template` SET `ScriptName`='npc_skovald_aegis_of_aggramar' WHERE `entry`=98364;
UPDATE `creature_template` SET `ScriptName`='npc_skovald_honored_ancestor' WHERE `entry`=101326;
UPDATE `creature_template` SET `ScriptName`='npc_skovald_flame_of_woe' WHERE `entry`=104822;
UPDATE `creature_template` SET `ScriptName`='npc_hov_valarjar_king' WHERE `entry` IN (95843,97081,97083,97084);

-- Odyn
UPDATE `creature_template` SET `ScriptName`='boss_odyn' WHERE `entry`=95676;
UPDATE `creature_template` SET `ScriptName`='npc_odyn_stormforged_obliterator' WHERE `entry`=102019;

-- Spell scripts
DELETE FROM `spell_script_names` WHERE `ScriptName` IN (
'spell_hov_thunderstrike',
'spell_hyrja_empowerment_tracker','spell_hyrja_expel_light','spell_hyrja_expel_light_aura','spell_hyrja_expel_light_damage','spell_hyrja_sanctify',
'spell_fenryr_scent_of_blood','spell_fenryr_scent_of_blood_fixate',
'spell_skovald_felblaze_rush','spell_skovald_ragnarok','spell_skovald_drop_aegis','spell_skovald_aegis_of_aggramar_player',
'spell_skovald_aegis_of_aggramar','spell_skovald_aegis_of_aggramar_shield','spell_skovald_aegis_check_cast',
'spell_odyn_radiant_tempest','spell_odyn_runic_brand');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(198599,'spell_hov_thunderstrike'),
(192130,'spell_hyrja_empowerment_tracker'),
(192044,'spell_hyrja_expel_light'),
(192048,'spell_hyrja_expel_light_aura'),
(192067,'spell_hyrja_expel_light_damage'),
(192158,'spell_hyrja_sanctify'),
(198800,'spell_fenryr_scent_of_blood'),
(196838,'spell_fenryr_scent_of_blood_fixate'),
(193658,'spell_skovald_felblaze_rush'),
(193826,'spell_skovald_ragnarok'),
(193991,'spell_skovald_drop_aegis'),
(193783,'spell_skovald_aegis_of_aggramar_player'),
(193983,'spell_skovald_aegis_of_aggramar'),
(193743,'spell_skovald_aegis_of_aggramar_shield'),
(202711,'spell_skovald_aegis_check_cast'),
(201006,'spell_odyn_radiant_tempest'),
(197963,'spell_odyn_runic_brand'),
(197964,'spell_odyn_runic_brand'),
(197965,'spell_odyn_runic_brand'),
(197966,'spell_odyn_runic_brand'),
(197967,'spell_odyn_runic_brand');

-- No areatrigger_create_properties rows exist for the Halls of Valor areatriggers (4492, 4865, 4866, 4885, 4960, 4961, 4985, 4987,
-- 5031, 5040, 5398, 5400, 5401, 5403, 5404, 5413, 5667), so no areatrigger scripts are bound here.
