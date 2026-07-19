-- RetailSystems: account balance and immutable BattlePay purchase history.

CREATE TABLE IF NOT EXISTS `retail_battlepay_balance` (
  `account_id` INT UNSIGNED NOT NULL,
  `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_battlepay_purchase` (
  `purchase_id` BIGINT UNSIGNED NOT NULL,
  `account_id` INT UNSIGNED NOT NULL,
  `character_guid` BIGINT UNSIGNED NOT NULL,
  `product_id` INT UNSIGNED NOT NULL,
  `base_price` BIGINT UNSIGNED NOT NULL,
  `user_price` BIGINT UNSIGNED NOT NULL,
  `status` INT UNSIGNED NOT NULL,
  `result_code` INT UNSIGNED NOT NULL DEFAULT 0,
  `created_at` BIGINT UNSIGNED NOT NULL,
  `delivered_at` BIGINT UNSIGNED NULL,
  `wallet_name` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`purchase_id`),
  KEY `idx_retail_battlepay_purchase_account` (`account_id`, `purchase_id`),
  KEY `idx_retail_battlepay_purchase_character` (`character_guid`, `purchase_id`),
  KEY `idx_retail_battlepay_purchase_status` (`status`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Example balance grant/update:
-- INSERT INTO `retail_battlepay_balance` (`account_id`, `balance`) VALUES (1, 100000)
-- ON DUPLICATE KEY UPDATE `balance` = VALUES(`balance`);
