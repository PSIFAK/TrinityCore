-- Quest 9361 "Helboar, the Other White Meat": Tainted Helboar Meat (23270) is not a quest objective, so QuestRequired never lets it drop.
-- Drop it while the quest is taken instead (ported from lun-core 26561d5e)
UPDATE `creature_loot_template` SET `QuestRequired`=0 WHERE `Item`=23270 AND `ItemType`=0 AND `Entry` IN (16863,16879,16880,16992);

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=1 AND `SourceEntry`=23270 AND `SourceGroup` IN (16863,16879,16880,16992);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(1,16863,23270,0,0,9,0,9361,0,0,0,0,0,'','Tainted Helboar Meat drops only while quest 9361 is taken'),
(1,16879,23270,0,0,9,0,9361,0,0,0,0,0,'','Tainted Helboar Meat drops only while quest 9361 is taken'),
(1,16880,23270,0,0,9,0,9361,0,0,0,0,0,'','Tainted Helboar Meat drops only while quest 9361 is taken'),
(1,16992,23270,0,0,9,0,9361,0,0,0,0,0,'','Tainted Helboar Meat drops only while quest 9361 is taken');
