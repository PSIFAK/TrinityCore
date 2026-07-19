-- RetailSystems: per-account BattlePay catalog synchronization timestamp.

CREATE TABLE IF NOT EXISTS `retail_battlepay_catalog_fetch` (
  `account_id` INT UNSIGNED NOT NULL,
  `last_fetch` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
