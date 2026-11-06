-- Kalecgos (156501) after the Darkmaul Citadel: gossip menus from the retail 12.1.5 sniff. Menu 39497 had TextID 0 and was
-- dropped at load, so he could not be talked to; 39498 (dragonflight aspects) was missing.
DELETE FROM `npc_text` WHERE `ID` IN (291293, 291295);
INSERT INTO `npc_text` (`ID`, `Probability0`, `Probability1`, `Probability2`, `Probability3`, `Probability4`, `Probability5`, `Probability6`, `Probability7`, `BroadcastTextID0`, `BroadcastTextID1`, `BroadcastTextID2`, `BroadcastTextID3`, `BroadcastTextID4`, `BroadcastTextID5`, `BroadcastTextID6`, `BroadcastTextID7`, `VerifiedBuild`) VALUES
(291293, 1, 0, 0, 0, 0, 0, 0, 0, 291293, 0, 0, 0, 0, 0, 0, 0, 70688),
(291295, 1, 0, 0, 0, 0, 0, 0, 0, 291295, 0, 0, 0, 0, 0, 0, 0, 70688);

DELETE FROM `gossip_menu` WHERE `MenuID` IN (39497, 39498);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(39497, 291293, 70688),
(39498, 291295, 70688);

UPDATE `gossip_menu_option` SET `ActionMenuID` = 39498 WHERE `MenuID` = 39497 AND `OptionID` = 1;
DELETE FROM `gossip_menu_option` WHERE `MenuID` = 39498;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(39498, 134084, 0, 0, '(Back)', 0, 0, 0, 39497, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 70688);
DELETE FROM `gossip_menu_option_locale` WHERE `MenuID` = 39498 AND `Locale` = 'ruRU';
INSERT INTO `gossip_menu_option_locale` (`MenuID`, `OptionID`, `Locale`, `OptionText`, `BoxText`) VALUES
(39498, 0, 'ruRU', '(Назад)', NULL);

-- Kalecgos on the island (244389, The End of the Beginning 55991): menu 39219 had TextID 0 as well and was dropped, so the
-- Dragon Isles travel option never showed; retail sends BroadcastText 289853 with it
DELETE FROM `npc_text` WHERE `ID` = 289853;
INSERT INTO `npc_text` (`ID`, `Probability0`, `Probability1`, `Probability2`, `Probability3`, `Probability4`, `Probability5`, `Probability6`, `Probability7`, `BroadcastTextID0`, `BroadcastTextID1`, `BroadcastTextID2`, `BroadcastTextID3`, `BroadcastTextID4`, `BroadcastTextID5`, `BroadcastTextID6`, `BroadcastTextID7`, `VerifiedBuild`) VALUES
(289853, 1, 0, 0, 0, 0, 0, 0, 0, 289853, 0, 0, 0, 0, 0, 0, 0, 70688);
DELETE FROM `gossip_menu` WHERE `MenuID` = 39219;
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(39219, 289853, 70688);
