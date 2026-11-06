-- client 12.1.5.70783: its build_auth_key rows are not known yet, worldserver accepts it with
-- RetailSystems.Auth.SkipMissingBuildAuthKey = 1
DELETE FROM `build_info` WHERE `build` IN (70783);
INSERT INTO `build_info` (`build`,`majorVersion`,`minorVersion`,`bugfixVersion`,`hotfixVersion`) VALUES
(70783,12,1,5,NULL);

UPDATE `realmlist` SET `gamebuild`=70783 WHERE `gamebuild`=70688;
