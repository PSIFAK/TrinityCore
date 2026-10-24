-- Court of Stars (map 1571) scripts, ported from lun-core (DekkCore) and rewritten

-- Instance script
DELETE FROM `instance_template` WHERE `map`=1571;
INSERT INTO `instance_template` (`map`, `parent`, `script`) VALUES
(1571, 0, 'instance_court_of_stars');

-- Patrol Captain Gerdo
UPDATE `creature_template` SET `ScriptName`='boss_patrol_captain_gerdo' WHERE `entry`=104215;
UPDATE `creature_template` SET `ScriptName`='npc_cos_vigilant_duskwatch' WHERE `entry`=104918;
UPDATE `creature_template` SET `ScriptName`='npc_cos_streetsweeper' WHERE `entry`=110403;

-- Talixae Flamewreath
UPDATE `creature_template` SET `ScriptName`='boss_talixae_flamewreath' WHERE `entry`=104217;
UPDATE `creature_template` SET `ScriptName`='npc_talixae_fel_covenant_guard' WHERE `entry` IN (104273, 104274, 104275);

-- Advisor Melandrus
UPDATE `creature_template` SET `ScriptName`='boss_advisor_melandrus' WHERE `entry`=104218;
UPDATE `creature_template` SET `ScriptName`='npc_melandrus_image_of_advisor_melandrus' WHERE `entry`=105754;
UPDATE `creature_template` SET `ScriptName`='npc_melandrus_enveloping_winds' WHERE `entry`=112687;

-- Trash and events
UPDATE `creature_template` SET `ScriptName`='npc_cos_signal_lantern' WHERE `entry`=105729;
UPDATE `creature_template` SET `ScriptName`='npc_cos_nightborne_boat' WHERE `entry`=105264;
UPDATE `creature_template` SET `ScriptName`='npc_cos_duskwatch_sentry' WHERE `entry`=104251;
UPDATE `creature_template` SET `ScriptName`='npc_cos_arcane_beacon' WHERE `entry`=104245;
UPDATE `creature_template` SET `ScriptName`='npc_cos_suspicious_noble' WHERE `entry`=107435;

-- Spell scripts
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_gerdo_resonant_slash', 'spell_gerdo_resonant_slash_damage', 'spell_gerdo_arcane_lockdown',
'spell_gerdo_summon_vigilant_nightwatch', 'spell_melandrus_enveloping_winds', 'spell_cos_righteous_indignation');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(207261, 'spell_gerdo_resonant_slash'),
(206574, 'spell_gerdo_resonant_slash_damage'),
(206580, 'spell_gerdo_resonant_slash_damage'),
(207278, 'spell_gerdo_arcane_lockdown'),
(210435, 'spell_gerdo_summon_vigilant_nightwatch'),
(224327, 'spell_melandrus_enveloping_winds'),
(213304, 'spell_cos_righteous_indignation');

-- 207278 - Arcane Lockdown: DB2 ProcTypeMask is PROC_FLAG_JUMP, but its aura types are not proc auras, so no proc entry is generated.
-- Every jump removes one stack (PROC_ATTR_USE_STACKS_FOR_CHARGES).
DELETE FROM `spell_proc` WHERE `SpellId`=207278;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(207278, 0, 0, 0, 0, 0, 0, 0x2000000, 0x0, 0x0, 0x0, 0x0, 0x10, 0, 0, 100, 0, 0);
