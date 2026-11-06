-- Exile's Reach: the Horde and Alliance copies of the island quests had AllowableRaces = all races and the Horde copies
-- mostly no prerequisites, so CMSG_UI_MAP_QUEST_LINES_REQUEST offered Horde quests to Alliance characters as available
-- (retail 12.1.5 answers UiMap 1409 with only the two quests the character can actually take). Quest lines from
-- QuestLineXQuest: Horde 1107/5369/5370/5459, Alliance 926/1070/5366/5367; A Shaman's Duty (59002) is in both.
UPDATE `quest_template` SET `AllowableRaces` = 12261800583900083122 WHERE `AllowableRaces` = 18446744073709551615 AND `ID` IN
(59926, 59927, 59928, 59929, 59930, 59931, 59932, 59933, 59934, 59935, 59937, 59938, 59939, 59940, 59941, 59942, 59943, 59944, 59945, 59946, 59947,
59948, 59949, 59950, 59951, 59952, 59953, 59954, 59955, 59956, 59957, 59958, 59961, 59965, 59967, 59968, 59970, 59971, 59972, 59975, 59978, 59979,
59980, 59981, 59984, 59985, 60162, 60174, 60343, 60344, 60345, 60346, 60347, 60348, 60349, 60350, 60351, 60352, 60353, 60355, 60357, 60359, 85679,
90840, 90842, 90843);
UPDATE `quest_template` SET `AllowableRaces` = 6130900294268439629 WHERE `AllowableRaces` = 18446744073709551615 AND `ID` IN
(54933, 54951, 54952, 55122, 55173, 55174, 55184, 55186, 55193, 55194, 55196, 55639, 55763, 55764, 55879, 55881, 55882, 55965, 55981, 55988, 55989,
55990, 55991, 55992, 56034, 56344, 56775, 56839, 58208, 58209, 58908, 58909, 58910, 58911, 58914, 58915, 58917, 58923, 58933, 58946, 58953, 58960,
58962, 59254, 59339, 59342, 59347, 59349, 59350, 59352, 59354, 59355, 59356, 59583, 59586, 59587, 59588, 59589, 59590, 59591, 59592, 59593, 59594,
60168, 85678, 87547, 87555, 87560, 87562);

-- Time Trials (55660) is an auto-push quest of a past event; Player::PushQuests gave it to every new character, retail
-- (12.1.5 sniff of a new hunter) does not
DELETE FROM `disables` WHERE `sourceType` = 1 AND `entry` = 55660;
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(1, 55660, 0, '', '', 'Time Trials - past event auto-push quest');
