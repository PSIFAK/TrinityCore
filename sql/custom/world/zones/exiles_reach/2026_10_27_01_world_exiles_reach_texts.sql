-- Exile's Reach: creature texts heard in the retail sniff and missing in TDB.
-- Text is the enUS broadcast text; BroadcastTextId resolved by ruRU text / sound kit. Not used by scripts yet (stage 6).

DELETE FROM `creature_text` WHERE (`CreatureID`, `BroadcastTextId`) IN ((152843,176098),(153211,177088),(153239,176113),(153581,176071),(153582,176072),(153582,176073),(153583,176069),(156626,291040),(156626,291042),(156799,290995),(156807,291054),(156833,178840),(156859,178864),(156886,190996),(156900,183958),(156900,172629),(156929,178915),(156943,185932),(156954,178935),(156986,178954),(157051,177651),(157114,179205),(157114,179244),(253758,296640));
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(152843, 0, 0, 'At least... I won\'t... starve...', 12, 0, 100, 0, 0, 150253, 0, 176098, 0, 'Harpy Boltcaller to Player'),
(153211, 0, 0, 'Hah! There! Come on, let\'s get outta here while we can!', 12, 0, 100, 0, 0, 152954, 0, 177088, 0, 'Meredy Huntswell to Player'),
(153239, 0, 0, 'How... me... die...', 12, 0, 100, 0, 0, 150267, 0, 176113, 0, 'Ogre Brute to Player'),
(153581, 0, 0, 'Me speak... too soon...', 12, 0, 100, 0, 0, 151445, 0, 176071, 0, 'Grunk to Player'),
(153582, 0, 0, 'Wug!', 12, 0, 100, 0, 0, 151449, 0, 176072, 0, 'Wug to Player'),
(153582, 1, 0, 'Wug.... wug...', 12, 0, 100, 0, 0, 151446, 0, 176073, 0, 'Wug to Player'),
(153583, 0, 0, 'Me... get... smash...', 12, 0, 100, 0, 0, 151451, 0, 176069, 0, 'Jugnug to Player'),
(156626, 1, 0, '$n! The sea was kind to you today.', 12, 0, 100, 3, 0, 297962, 0, 291040, 0, 'Lady Jaina Proudmoore to Player'),
(156626, 2, 0, 'Meet Captain Garrick up ahead. I\'m going to search the island for more survivors.', 12, 0, 100, 0, 0, 297965, 0, 291042, 0, 'Lady Jaina Proudmoore to Player'),
(156799, 0, 0, 'Honestly, the indignity of it all...', 12, 0, 100, 0, 0, 297991, 0, 290995, 0, 'Wrathion to Player'),
(156807, 0, 0, 'Let\'s move. We need to put a stop to that ritual before it\'s too late.', 12, 0, 100, 0, 0, 297976, 0, 291054, 0, 'Lady Jaina Proudmoore to Player'),
(156833, 0, 0, 'Kee-La and I will scout ahead. We\'ll meet you to the north, just outside the harpy den.', 12, 0, 100, 0, 0, 153031, 0, 178840, 0, 'Henry Garrick to Player'),
(156859, 0, 0, 'The captain will be thrilled! Let\'s get back to camp and deliver the good news.', 12, 0, 100, 0, 0, 153036, 0, 178864, 0, 'Henry Garrick to Player'),
(156886, 0, 0, 'Here\'s a taste o\' Polymorph!', 12, 0, 100, 0, 0, 152943, 0, 190996, 0, 'Meredy Huntswell to Player'),
(156900, 0, 0, 'If... I die... so... do you!', 12, 0, 100, 0, 0, 152998, 0, 183958, 0, 'Hrun the Exiled to Player'),
(156900, 1, 0, 'You will be kindling for the flame!', 12, 0, 100, 0, 0, 152997, 0, 172629, 0, 'Hrun the Exiled to Player'),
(156929, 0, 0, 'You saved my life! Allow me to return the favor. Hop on!', 12, 0, 100, 0, 0, 152936, 0, 178915, 0, 'Ralia Dreamchaser to Player'),
(156943, 0, 0, 'If ye feel a strange tingle, that means the illusion is working!', 12, 0, 100, 0, 0, 152955, 0, 185932, 0, 'Meredy Huntswell to Player'),
(156954, 0, 0, 'Thank you for saving me... but we still need to stop Gor\'groth!', 12, 0, 100, 0, 0, 153064, 0, 178935, 0, 'Captain Kelra to Player'),
(156986, 0, 0, 'Boss... counting... on me...', 12, 0, 100, 0, 0, 150272, 0, 178954, 0, 'Ogre Taskmaster to Player'),
(157051, 1, 0, 'Let\'s move into sparring positions. I\'ll let you have the first strike.', 12, 0, 100, 0, 0, 152846, 0, 177651, 0, 'Alliance Sparring Partner to Player'),
(157114, 0, 0, 'The spell is weakening... the Light returns to me!', 12, 0, 100, 0, 0, 153069, 0, 179205, 0, 'Lightspawn to Player'),
(157114, 1, 0, 'Ahh, freedom! You have my gratitude.', 12, 0, 100, 0, 0, 153070, 0, 179244, 0, 'Lightspawn to Player'),
(253758, 0, 0, 'I\'ll do what I can to forestall the ritual, but don\'t keep me waiting. I would hate to get captured again.', 12, 0, 100, 1, 0, 297995, 0, 296640, 0, 'Wrathion to Player');
