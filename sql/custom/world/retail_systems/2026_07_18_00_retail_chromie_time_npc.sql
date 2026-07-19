-- RetailSystems: expose the native Chromie Time UI through the official
-- Stormwind Chromie (creature 167032, gossip menu 25426).
--
-- INSERT IGNORE deliberately leaves future TrinityCore data untouched when
-- upstream supplies the retail option for this menu.

INSERT IGNORE INTO `creature_template_gossip`
  (`CreatureID`, `MenuID`, `VerifiedBuild`)
VALUES
  (167032, 25426, 68453);

INSERT IGNORE INTO `gossip_menu_option`
  (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`,
   `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`,
   `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`,
   `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`)
VALUES
  (25426, 0, 0, 40, 'Select a timeline.',
   0, 0, 0, 0, 0,
   NULL, 0, 0, '',
   0, NULL, NULL, 68453);

INSERT IGNORE INTO `gossip_menu_option_locale`
  (`MenuID`, `OptionID`, `Locale`, `OptionText`, `BoxText`)
VALUES
  (25426, 0, 'ruRU', 'Выбрать временную линию.', '');
