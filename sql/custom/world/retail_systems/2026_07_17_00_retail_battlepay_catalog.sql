-- RetailSystems: BattlePay catalog for the 12.0.7 client protocol.
-- Prices are the raw 64-bit fixed-point values sent to the client.

CREATE TABLE IF NOT EXISTS `retail_battlepay_group` (
  `group_id` INT UNSIGNED NOT NULL,
  `icon_file_data_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `display_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `ordering` INT UNSIGNED NOT NULL DEFAULT 0,
  `flags` INT UNSIGNED NOT NULL DEFAULT 0,
  `parent_group_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(255) NOT NULL DEFAULT '',
  `disabled_description` VARCHAR(1024) NOT NULL DEFAULT '',
  `active` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`group_id`),
  KEY `idx_retail_battlepay_group_order` (`active`, `ordering`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_battlepay_product` (
  `product_id` INT UNSIGNED NOT NULL,
  `group_id` INT UNSIGNED NOT NULL,
  `normal_price` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `current_price` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `product_type` INT UNSIGNED NOT NULL DEFAULT 0,
  `flags` INT UNSIGNED NOT NULL DEFAULT 0,
  `required_deliverable_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `eligibility` INT UNSIGNED NOT NULL DEFAULT 0,
  `pmt_product_id` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `ordering` INT UNSIGNED NOT NULL DEFAULT 0,
  `shop_flags` INT UNSIGNED NOT NULL DEFAULT 0,
  `banner_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `display_file_data_id` INT UNSIGNED NULL,
  `display_model_scene_id` INT UNSIGNED NULL,
  `name_1` TEXT NULL,
  `name_2` TEXT NULL,
  `name_3` TEXT NULL,
  `tooltip` TEXT NULL,
  `instructions` TEXT NULL,
  `display_flags` INT UNSIGNED NULL,
  `override_text_color` INT UNSIGNED NULL,
  `override_texture` INT UNSIGNED NULL,
  `override_background` INT UNSIGNED NULL,
  `disclaimer` TEXT NULL,
  `nydus_link` TEXT NULL,
  `battlepay_card_type` INT UNSIGNED NOT NULL DEFAULT 0,
  `item_quantity` INT UNSIGNED NOT NULL DEFAULT 0,
  `active` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`product_id`),
  KEY `idx_retail_battlepay_product_group` (`active`, `group_id`, `ordering`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_battlepay_deliverable` (
  `deliverable_id` INT UNSIGNED NOT NULL,
  `product_id` INT UNSIGNED NOT NULL,
  `client_type` INT UNSIGNED NOT NULL DEFAULT 0,
  `delivery_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=item, 1=spell, 2=mount, 3=toy',
  `entry` INT UNSIGNED NOT NULL COMMENT 'Server item/spell/mount/toy identifier selected by delivery_type',
  `quantity` INT UNSIGNED NOT NULL DEFAULT 1,
  `flags` INT UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(255) NOT NULL DEFAULT '',
  `item_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `mount_spell_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `battle_pet_creature_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `boost_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `trans_item_modified_appearance_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `transmog_set_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `char_title_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `spell_item_enchantment_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `warband_scene_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `display_file_data_id` INT UNSIGNED NULL,
  `display_model_scene_id` INT UNSIGNED NULL,
  `display_name` VARCHAR(1023) NOT NULL DEFAULT '',
  `display_tooltip` VARCHAR(4096) NOT NULL DEFAULT '',
  `ordering` INT UNSIGNED NOT NULL DEFAULT 0,
  `active` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`deliverable_id`),
  KEY `idx_retail_battlepay_deliverable_product` (`active`, `product_id`, `ordering`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `retail_battlepay_product_bundle` (
  `product_id` INT UNSIGNED NOT NULL,
  `bundled_product_id` INT UNSIGNED NOT NULL,
  `ordering` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`product_id`, `bundled_product_id`),
  KEY `idx_retail_battlepay_bundle_order` (`product_id`, `ordering`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Intentionally no seeded products: catalog identifiers and rewards are realm policy.
-- Minimal example (replace item 6948 and the text before enabling in production):
-- INSERT INTO `retail_battlepay_group` (`group_id`, `ordering`, `name`) VALUES (1, 1, 'Предметы');
-- INSERT INTO `retail_battlepay_product`
--   (`product_id`, `group_id`, `normal_price`, `current_price`, `ordering`, `name_1`, `tooltip`)
-- VALUES (1001, 1, 10000, 10000, 1, 'Тестовый предмет', 'Тестовая покупка');
-- INSERT INTO `retail_battlepay_deliverable`
--   (`deliverable_id`, `product_id`, `delivery_type`, `entry`, `quantity`, `name`)
-- VALUES (2001, 1001, 0, 6948, 1, 'Тестовый предмет');
