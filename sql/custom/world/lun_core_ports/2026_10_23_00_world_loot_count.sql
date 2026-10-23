-- Allow loot MinCount/MaxCount above 255 (ported from lun-core b1da3e9b)
ALTER TABLE `creature_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `disenchant_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `fishing_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `gameobject_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `item_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `mail_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `milling_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `pickpocketing_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `prospecting_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `reference_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `skinning_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
ALTER TABLE `spell_loot_template` MODIFY `MinCount` int unsigned NOT NULL DEFAULT 1, MODIFY `MaxCount` int unsigned NOT NULL DEFAULT 1;
