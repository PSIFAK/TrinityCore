-- RetailSystems: minimal BattlePay catalog used to verify the 12.0.7 shop,
-- purchase confirmation, balance debit and item delivery end to end.
-- Product 1001 delivers 20 Linen Cloth (item 2589).

INSERT INTO `retail_battlepay_group`
  (`group_id`, `icon_file_data_id`, `display_type`, `ordering`, `flags`,
   `parent_group_id`, `name`, `disabled_description`, `active`)
VALUES
  (1, 237429, 0, 1, 10, 0, 'Тестовые товары', '', 1)
ON DUPLICATE KEY UPDATE
  `icon_file_data_id` = VALUES(`icon_file_data_id`),
  `display_type` = VALUES(`display_type`),
  `ordering` = VALUES(`ordering`),
  `flags` = VALUES(`flags`),
  `name` = VALUES(`name`),
  `disabled_description` = VALUES(`disabled_description`),
  `active` = VALUES(`active`);

INSERT INTO `retail_battlepay_product`
  (`product_id`, `group_id`, `normal_price`, `current_price`, `product_type`,
   `flags`, `required_deliverable_id`, `eligibility`, `pmt_product_id`,
   `ordering`, `shop_flags`, `banner_type`, `display_file_data_id`,
   `display_model_scene_id`, `name_1`, `name_2`, `name_3`,
   `tooltip`, `instructions`, `display_flags`, `battlepay_card_type`,
   `item_quantity`, `active`)
VALUES
  (1001, 1, 10000, 10000, 0,
   15, 0, 0, 1001,
   1, 0, 0, 2823166, 10, '20 ед. льняного материала', '',
   'Тестовая покупка BattlePay. Предмет будет добавлен в сумки персонажа.',
   '', '', 130, 0, 20, 1)
ON DUPLICATE KEY UPDATE
  `group_id` = VALUES(`group_id`),
  `normal_price` = VALUES(`normal_price`),
  `current_price` = VALUES(`current_price`),
  `product_type` = VALUES(`product_type`),
  `flags` = VALUES(`flags`),
  `required_deliverable_id` = VALUES(`required_deliverable_id`),
  `ordering` = VALUES(`ordering`),
  `shop_flags` = VALUES(`shop_flags`),
  `banner_type` = VALUES(`banner_type`),
  `display_file_data_id` = VALUES(`display_file_data_id`),
  `display_model_scene_id` = VALUES(`display_model_scene_id`),
  `name_1` = VALUES(`name_1`),
  `name_3` = VALUES(`name_3`),
  `tooltip` = VALUES(`tooltip`),
  `instructions` = VALUES(`instructions`),
  `display_flags` = VALUES(`display_flags`),
  `item_quantity` = VALUES(`item_quantity`),
  `active` = VALUES(`active`);

INSERT INTO `retail_battlepay_deliverable`
  (`deliverable_id`, `product_id`, `client_type`, `delivery_type`, `entry`,
   `quantity`, `flags`, `name`, `item_id`, `display_name`, `display_tooltip`,
   `ordering`, `active`)
VALUES
  (2001, 1001, 14, 0, 2589,
   20, 2, 'Льняной материал', 2589, '', '', 1, 1)
ON DUPLICATE KEY UPDATE
  `product_id` = VALUES(`product_id`),
  `client_type` = VALUES(`client_type`),
  `delivery_type` = VALUES(`delivery_type`),
  `entry` = VALUES(`entry`),
  `quantity` = VALUES(`quantity`),
  `flags` = VALUES(`flags`),
  `name` = VALUES(`name`),
  `item_id` = VALUES(`item_id`),
  `display_name` = VALUES(`display_name`),
  `display_tooltip` = VALUES(`display_tooltip`),
  `ordering` = VALUES(`ordering`),
  `active` = VALUES(`active`);
