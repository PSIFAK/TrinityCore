-- Skyriding abilities (ported from lun-core spell_dragonriding.cpp, reworked)
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_skyriding_lift_off','spell_skyriding_skyward_ascent','spell_skyriding_surge_forward','spell_skyriding_switch_flight_style');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(374763,'spell_skyriding_lift_off'),
(372610,'spell_skyriding_skyward_ascent'),
(372608,'spell_skyriding_surge_forward'),
(436854,'spell_skyriding_switch_flight_style');
