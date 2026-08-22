SELECT  `settings`.* FROM `settings` WHERE `settings`.`name` = 'lead_status' LIMIT 1
SELECT  `accounts`.* FROM `accounts` INNER JOIN `account_contacts` ON `accounts`.`id` = `account_contacts`.`account_id` WHERE `account_contacts`.`contact_id` = 1234 LIMIT 1
SELECT `permissions`.`asset_type`, `permissions`.`asset_id` FROM `permissions` WHERE `permissions`.`user_id` = 2939 AND `permissions`.`asset_type` IN ('Account', 'Campaign', 'Contact', 'Lead', 'Opportunity')
SELECT  `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 2939) OR ((`accounts`.`user_id` = 2939) OR (`accounts`.`access` = 'Public'))) ORDER BY `accounts`.`name` ASC LIMIT 25
SELECT `users`.* FROM `users` ORDER BY first_name, last_name, email
SELECT `tags`.* FROM `tags`
SELECT `taggings`.* FROM `taggings` WHERE `taggings`.`taggable_id` = 1234 AND `taggings`.`taggable_type` = 'Contact'
SELECT `tags`.* FROM `tags` INNER JOIN `taggings` ON `tags`.`id` = `taggings`.`tag_id` WHERE `taggings`.`taggable_id` = 1234 AND `taggings`.`taggable_type` = 'Contact' AND (taggings.context = 'tags' AND taggings.tagger_id IS NULL)
SELECT `tags`.`id` FROM `tags` INNER JOIN `taggings` ON `tags`.`id` = `taggings`.`tag_id` WHERE `taggings`.`taggable_id` = 1234 AND `taggings`.`taggable_type` = 'Contact' AND `taggings`.`context` = 'tags'
SELECT `field_groups`.* FROM `field_groups` WHERE `field_groups`.`klass_name` = 'Contact' AND (tag_id IS NULL OR tag_id IN (NULL)) ORDER BY `field_groups`.`position` ASC
SELECT  `addresses`.* FROM `addresses` WHERE `addresses`.`addressable_id` = 1234 AND `addresses`.`addressable_type` = 'Contact' AND `addresses`.`address_type` = 'Business' LIMIT 1
SELECT `users`.* FROM `users`
SELECT `permissions`.* FROM `permissions` WHERE `permissions`.`asset_id` = 1234 AND `permissions`.`asset_type` = 'Contact'
SELECT `groups`.* FROM `groups`
SELECT  `users`.* FROM `users` WHERE `users`.`id` = 3056 ORDER BY `users`.`id` ASC LIMIT 1
SELECT `groups`.`id` FROM `groups` INNER JOIN `groups_users` ON `groups`.`id` = `groups_users`.`group_id` WHERE `groups_users`.`user_id` = 3056
SELECT  `campaigns`.* FROM `campaigns` WHERE `campaigns`.`id` = 1907 LIMIT 1
SELECT  `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 3056) OR ((`campaigns`.`user_id` = 3056) OR (`campaigns`.`access` = 'Public'))) ORDER BY `campaigns`.`name` ASC LIMIT 25
SELECT COUNT(*) FROM (SELECT DISTINCT `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 237) OR ((`accounts`.`user_id` = 237) OR (`accounts`.`access` = 'Public'))) ORDER BY accounts.created_at DESC) subquery_for_count
SELECT  1 AS one FROM `users` WHERE `users`.`email` = 'sandi_beatty@johnstonhoeger.name' LIMIT 1
SELECT  1 AS one FROM `users` WHERE `users`.`username` = 'ophelia.gorczany2158' LIMIT 1
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`name` =  'Repellendus magnam odit eum occaecati tempora sunt aspernatur de' AND `campaigns`.`user_id` = 2085 AND `campaigns`.`deleted_at` IS NULL LIMIT 1
SELECT `field_groups`.* FROM `field_groups` WHERE `field_groups`.`klass_name` = 'Task' ORDER BY `field_groups`.`position` ASC
SELECT  `users`.* FROM `users` WHERE `users`.`id` = 273 LIMIT 1
SELECT  `preferences`.* FROM `preferences` WHERE `preferences`.`name` = 'contacts_sort_by' AND `preferences`.`user_id` = 688 LIMIT 1
SELECT COUNT(*) FROM (SELECT DISTINCT `contacts`.* FROM `contacts` WHERE ((`contacts`.`assigned_to` = 688) OR ((`contacts`.`user_id` = 688) OR (`contacts`.`access` = 'Public'))) ORDER BY contacts.created_at DESC) subquery_for_count
SELECT  DISTINCT `contacts`.* FROM `contacts` WHERE ((`contacts`.`assigned_to` = 688) OR ((`contacts`.`user_id` = 688) OR (`contacts`.`access` = 'Public'))) ORDER BY contacts.created_at DESC LIMIT 20 OFFSET 820
SELECT COUNT(DISTINCT `contacts`.`id`) FROM `contacts` WHERE ((`contacts`.`assigned_to` = 688) OR ((`contacts`.`user_id` = 688) OR (`contacts`.`access` = 'Public')))
SELECT  1 AS one FROM `lists` WHERE `lists`.`user_id` IS NULL LIMIT 1
SELECT  1 AS one FROM `lists` WHERE `lists`.`user_id` = 3075 LIMIT 1
SELECT  1 AS one FROM `versions` WHERE `versions`.`whodunnit` = '3075' AND `versions`.`item_type` IN ('Account', 'Campaign', 'Contact', 'Lead', 'Opportunity') LIMIT 1 OFFSET 0
SELECT COUNT(*) FROM (SELECT DISTINCT `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 238) OR ((`accounts`.`user_id` = 238) OR (`accounts`.`access` = 'Public'))) AND (category IN ('customer','vendor')) ORDER BY accounts.created_at DESC) subquery_for_count
SELECT COUNT(*) FROM (SELECT DISTINCT `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 1131) OR ((`leads`.`user_id` = 1131) OR (`leads`.`access` = 'Public'))) AND (status IN ('new')) ORDER BY leads.created_at DESC) subquery_for_count
SELECT  `contacts`.* FROM `contacts` WHERE `contacts`.`lead_id` = 352 LIMIT 1
SELECT `tasks`.* FROM `tasks` WHERE `tasks`.`asset_id` = 352 AND `tasks`.`asset_type` = 'Lead'
SELECT  `addresses`.* FROM `addresses` WHERE `addresses`.`addressable_id` = 352 AND `addresses`.`addressable_type` = 'Lead' AND (address_type='Business') LIMIT 1
SELECT `addresses`.* FROM `addresses` WHERE `addresses`.`addressable_id` = 352 AND `addresses`.`addressable_type` = 'Lead'
SELECT `comments`.* FROM `comments` WHERE `comments`.`commentable_id` = 352 AND `comments`.`commentable_type` = 'Lead'
SELECT `taggings`.* FROM `taggings` WHERE `taggings`.`taggable_id` = 352 AND `taggings`.`taggable_type` = 'Lead' AND `taggings`.`context` = 'tags'
SELECT  `opportunities`.* FROM `opportunities` WHERE `opportunities`.`id` = 1111 LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`assigned_to` = 35 LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`user_id` = 35 LIMIT 1
SELECT DISTINCT `opportunities`.* FROM `opportunities` INNER JOIN `account_opportunities` ON `opportunities`.`id` = `account_opportunities`.`opportunity_id` WHERE `account_opportunities`.`account_id` = 2112 AND (opportunities.stage = 'won') ORDER BY opportunities.id DESC
SELECT DISTINCT `opportunities`.* FROM `opportunities` INNER JOIN `account_opportunities` ON `opportunities`.`id` = `account_opportunities`.`opportunity_id` WHERE `account_opportunities`.`account_id` = 2112 AND (opportunities.stage IS NULL OR (opportunities.stage != 'won' AND opportunities.stage != 'lost')) ORDER BY opportunities.id DESC
SELECT `tags`.* FROM `tags` INNER JOIN `taggings` ON `tags`.`id` = `taggings`.`tag_id` WHERE `taggings`.`taggable_id` = 2112 AND `taggings`.`taggable_type` = 'Account' AND `taggings`.`context` = 'tags'
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`assigned_to` = 33 LIMIT 1
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`user_id` = 33 LIMIT 1
SELECT  1 AS one FROM `leads` WHERE `leads`.`assigned_to` = 33 LIMIT 1
SELECT  1 AS one FROM `leads` WHERE `leads`.`user_id` = 33 LIMIT 1
SELECT  1 AS one FROM `contacts` WHERE `contacts`.`assigned_to` = 33 LIMIT 1
SELECT  1 AS one FROM `contacts` WHERE `contacts`.`user_id` = 33 LIMIT 1
SELECT  1 AS one FROM `opportunities` WHERE (assigned_to = 33) LIMIT 1
SELECT  1 AS one FROM `opportunities` WHERE (user_id = 33) LIMIT 1
SELECT  1 AS one FROM `comments` WHERE `comments`.`user_id` = 33 LIMIT 1
SELECT  1 AS one FROM `tasks` WHERE `tasks`.`assigned_to` = 33 LIMIT 1
SELECT  1 AS one FROM `tasks` WHERE `tasks`.`user_id` = 33 LIMIT 1
SELECT  `avatars`.* FROM `avatars` WHERE `avatars`.`entity_id` = 33 AND `avatars`.`entity_type` = 'User' LIMIT 1
SELECT `permissions`.* FROM `permissions` WHERE `permissions`.`user_id` = 33
SELECT `preferences`.* FROM `preferences` WHERE `preferences`.`user_id` = 33
SELECT  `field_groups`.* FROM `field_groups` WHERE `field_groups`.`tag_id` = 1 AND `field_groups`.`klass_name` = 'Contact' LIMIT 1
SELECT  `users`.* FROM `users` WHERE ((lower(email) = 'aaron@example.com' OR lower(alt_email) = 'aaron@example.com') AND suspended_at IS NULL) LIMIT 1
SELECT  `contacts`.* FROM `contacts` WHERE (first_name LIKE '%Cindy' AND last_name LIKE '%Cluster') ORDER BY `contacts`.`id` ASC LIMIT 1
SELECT  `tasks`.* FROM `tasks` WHERE (user_id = 1619 OR assigned_to = 1619) AND `tasks`.`id` = 1027 LIMIT 1
SELECT  `preferences`.* FROM `preferences` WHERE `preferences`.`name` = 'locale' AND `preferences`.`user_id` IS NULL LIMIT 1
SELECT  `accounts`.* FROM `accounts` INNER JOIN `account_opportunities` ON `accounts`.`id` = `account_opportunities`.`account_id` WHERE `account_opportunities`.`opportunity_id` = 42 LIMIT 1
SELECT `users`.* FROM `users` WHERE (id != 2821) ORDER BY first_name, last_name, email
SELECT  `accounts`.* FROM `accounts` WHERE ((lower(email) = 'ben@example.com')) ORDER BY `accounts`.`id` ASC LIMIT 1
SELECT  `contacts`.* FROM `contacts` WHERE ((lower(email) = 'ben@example.com')) ORDER BY `contacts`.`id` ASC LIMIT 1
SELECT  `contacts`.* FROM `contacts` WHERE ((lower(alt_email) = 'ben@example.com')) ORDER BY `contacts`.`id` ASC LIMIT 1
SELECT  `leads`.* FROM `leads` WHERE ((lower(email) = 'ben@example.com')) ORDER BY `leads`.`id` ASC LIMIT 1
SELECT  `leads`.* FROM `leads` WHERE ((lower(alt_email) = 'ben@example.com')) ORDER BY `leads`.`id` ASC LIMIT 1
SELECT  `accounts`.* FROM `accounts` WHERE ((lower(email) like '%example.com' OR lower(website) like '%example.com%')) ORDER BY `accounts`.`id` ASC LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`name` =  'Example.com' AND `accounts`.`deleted_at` IS NULL LIMIT 1
SELECT  `tasks`.* FROM `tasks` WHERE `tasks`.`id` = 1032 LIMIT 1
SELECT `versions`.* FROM `versions`
SELECT  `accounts`.* FROM `accounts` WHERE `accounts`.`id` = 1278 LIMIT 1
SELECT  `accounts`.* FROM `accounts` WHERE `accounts`.`id` = 1279 ORDER BY `accounts`.`id` ASC LIMIT 1
SELECT  `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 1194) OR ((`accounts`.`user_id` = 1194) OR (`accounts`.`access` = 'Public'))) AND `accounts`.`id` = 163 LIMIT 1
SELECT  `leads`.* FROM `leads` WHERE `leads`.`id` = 370 LIMIT 1
SELECT COUNT(*) FROM (SELECT DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1496) OR ((`opportunities`.`user_id` = 1496) OR (`opportunities`.`access` = 'Public'))) ORDER BY opportunities.name ASC) subquery_for_count
SELECT `comments`.`id` FROM `comments` WHERE `comments`.`commentable_id` = 42 AND `comments`.`commentable_type` = 'Campaign'
SELECT `emails`.`id` FROM `emails` WHERE `emails`.`mediator_id` = 42 AND `emails`.`mediator_type` = 'Campaign'
SELECT  `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 929) OR ((`leads`.`user_id` = 929) OR (`leads`.`access` = 'Public'))) AND `leads`.`id` = 327 LIMIT 1
SELECT  `account_contacts`.* FROM `account_contacts` WHERE `account_contacts`.`contact_id` = 42 LIMIT 1
SELECT COUNT(*) FROM `leads` WHERE ((`leads`.`assigned_to` = 980) OR ((`leads`.`user_id` = 980) OR (`leads`.`access` = 'Public')))
SELECT COUNT(*) AS count_all, `leads`.`status` AS leads_status FROM `leads` WHERE ((`leads`.`assigned_to` = 980) OR ((`leads`.`user_id` = 980) OR (`leads`.`access` = 'Public'))) AND `leads`.`status` IN ('new', 'contacted', 'converted', 'rejected') GROUP BY `leads`.`status`
SELECT COUNT(*) FROM (SELECT DISTINCT `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 980) OR ((`leads`.`user_id` = 980) OR (`leads`.`access` = 'Public'))) ORDER BY leads.created_at DESC) subquery_for_count
SELECT  DISTINCT `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 980) OR ((`leads`.`user_id` = 980) OR (`leads`.`access` = 'Public'))) ORDER BY leads.created_at DESC LIMIT 20 OFFSET 0
SELECT `taggings`.`id` AS t0_r0, `taggings`.`tag_id` AS t0_r1, `taggings`.`taggable_id` AS t0_r2, `taggings`.`tagger_id` AS t0_r3, `taggings`.`tagger_type` AS t0_r4, `taggings`.`taggable_type` AS t0_r5, `taggings`.`context` AS t0_r6, `taggings`.`created_at` AS t0_r7, `tags`.`id` AS t1_r0, `tags`.`name` AS t1_r1, `tags`.`taggings_count` AS t1_r2 FROM `taggings` LEFT OUTER JOIN `tags` ON `tags`.`id` = `taggings`.`tag_id` WHERE `taggings`.`taggable_type` = 'Lead' AND `taggings`.`context` = 'tags' AND `taggings`.`taggable_id` = 348

SELECT COUNT(DISTINCT `leads`.`id`) FROM `leads` WHERE ((`leads`.`assigned_to` = 983) OR ((`leads`.`user_id` = 983) OR (`leads`.`access` = 'Public')))
SELECT  `accounts`.* FROM `accounts` WHERE `accounts`.`name` = 'Hello' LIMIT 1
SELECT  1 AS one FROM `permissions` WHERE `permissions`.`user_id` = 7 AND `permissions`.`group_id` IS NULL AND `permissions`.`asset_id` = 42 AND `permissions`.`asset_type` = 'Contact' LIMIT 1
SELECT COUNT(*) FROM (SELECT DISTINCT `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 239) OR ((`accounts`.`user_id` = 239) OR (`accounts`.`access` = 'Public'))) AND (`accounts`.`name` LIKE '%second%' OR `accounts`.`email` LIKE '%second%') ORDER BY accounts.created_at DESC) subquery_for_count
SELECT COUNT(*) FROM `tasks` WHERE ((user_id = 1621 AND assigned_to IS NULL) OR assigned_to = 1621) AND (due_at IS NULL AND bucket = 'due_asap') AND (completed_at IS NULL)
SELECT COUNT(*) FROM (SELECT DISTINCT `accounts`.* FROM `accounts` WHERE ((`accounts`.`assigned_to` = 358) OR ((`accounts`.`user_id` = 358) OR (`accounts`.`access` = 'Public'))) ORDER BY accounts.name ASC) subquery_for_count
SELECT  `tags`.* FROM `tags` WHERE `tags`.`name` = 'laree_blick' LIMIT 1
SELECT COUNT(*) FROM `tasks` WHERE (user_id = 3225 AND assigned_to IS NOT NULL AND assigned_to != 3225) AND (due_at IS NULL AND bucket = 'due_asap') AND (completed_at IS NULL)
SELECT COUNT(*) FROM `groups` INNER JOIN `groups_users` ON `groups`.`id` = `groups_users`.`group_id` WHERE `groups_users`.`user_id` = 1086
SELECT  1 AS one FROM `users` WHERE `users`.`email` = 'jacquelin@franecki.info' AND (`users`.`id` != 1077) LIMIT 1
SELECT  1 AS one FROM `users` WHERE `users`.`username` = 'cary2954' AND (`users`.`id` != 1077) LIMIT 1
SELECT  1 AS one FROM `users` WHERE `users`.`username` IS NULL LIMIT 1
SELECT  `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 193) OR ((`campaigns`.`user_id` = 193) OR (`campaigns`.`access` = 'Public'))) AND `campaigns`.`id` = 33 LIMIT 1
SELECT  `contacts`.* FROM `contacts` WHERE ((`contacts`.`assigned_to` = 195) OR ((`contacts`.`user_id` = 195) OR (`contacts`.`access` = 'Public'))) AND `contacts`.`id` = 9 LIMIT 1
SELECT  `opportunities`.* FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 202) OR ((`opportunities`.`user_id` = 202) OR (`opportunities`.`access` = 'Public'))) AND `opportunities`.`id` = 9 LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`name` =  'Nader, Lind and Koepp57' AND (`accounts`.`id` != 1285) AND `accounts`.`deleted_at` IS NULL LIMIT 1
SELECT COUNT(*) AS count_all, `campaigns`.`status` AS campaigns_status FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 369) OR ((`campaigns`.`user_id` = 369) OR (`campaigns`.`access` = 'Public'))) AND `campaigns`.`status` IN ('planned', 'started', 'completed', 'on_hold', 'called_off') GROUP BY `campaigns`.`status`
SELECT COUNT(*) FROM `tasks` WHERE ((user_id = 1627 AND assigned_to IS NULL) OR assigned_to = 1627) AND (due_at IS NOT NULL AND due_at < '2020-05-15 00:00:00') AND (completed_at IS NULL)
SELECT COUNT(*) FROM `tasks` WHERE ((user_id = 1627 AND assigned_to IS NULL) OR assigned_to = 1627) AND (due_at >= '2020-05-15 00:00:00' AND due_at < '2020-05-16 00:00:00') AND (completed_at IS NULL)
SELECT COUNT(*) FROM `tasks` WHERE ((user_id = 1627 AND assigned_to IS NULL) OR assigned_to = 1627) AND ((due_at IS NULL AND bucket = 'due_later') OR due_at >= '2020-05-25 23:59:59.999999') AND (completed_at IS NULL)
SELECT COUNT(*) FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 410) OR ((`campaigns`.`user_id` = 410) OR (`campaigns`.`access` = 'Public')))
SELECT COUNT(*) FROM (SELECT DISTINCT `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 410) OR ((`campaigns`.`user_id` = 410) OR (`campaigns`.`access` = 'Public'))) ORDER BY campaigns.created_at DESC) subquery_for_count
SELECT  DISTINCT `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 410) OR ((`campaigns`.`user_id` = 410) OR (`campaigns`.`access` = 'Public'))) ORDER BY campaigns.created_at DESC LIMIT 20 OFFSET 0
SELECT COUNT(DISTINCT `campaigns`.`id`) FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 411) OR ((`campaigns`.`user_id` = 411) OR (`campaigns`.`access` = 'Public')))
SELECT  DISTINCT `opportunities`.* FROM `opportunities` INNER JOIN `contact_opportunities` ON `opportunities`.`id` = `contact_opportunities`.`opportunity_id` WHERE `contact_opportunities`.`contact_id` = 42 ORDER BY opportunities.id DESC, updated_at desc LIMIT 20 OFFSET 0
SELECT  `versions`.* FROM `versions` WHERE ((item_id = 42 AND item_type = 'Contact') OR (related_id = 42 AND related_type = 'Contact')) AND (event NOT IN ('view')) ORDER BY created_at DESC LIMIT 20 OFFSET 0
SELECT COUNT(*) AS count_all, `opportunities`.`stage` AS opportunities_stage FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1148) OR ((`opportunities`.`user_id` = 1148) OR (`opportunities`.`access` = 'Public'))) AND `opportunities`.`stage` IN ('prospecting', 'analysis', 'presentation', 'proposal', 'negotiation', 'final_review', 'won', 'lost') GROUP BY `opportunities`.`stage`
SELECT `emails`.* FROM `emails` WHERE `emails`.`mediator_id` = 42 AND `emails`.`mediator_type` = 'Lead'
SELECT `leads`.* FROM `leads` WHERE `leads`.`campaign_id` = 112 ORDER BY id DESC
SELECT `opportunities`.* FROM `opportunities` WHERE `opportunities`.`campaign_id` = 112 ORDER BY id DESC
SELECT COUNT(*) FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1143) OR ((`opportunities`.`user_id` = 1143) OR (`opportunities`.`access` = 'Public')))
SELECT `tasks`.* FROM `tasks` WHERE ((user_id = 1558 AND assigned_to IS NULL) OR assigned_to = 1558) AND (due_at IS NOT NULL AND due_at < '2020-05-15 00:00:00') AND (completed_at IS NULL) ORDER BY name ASC, tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE ((user_id = 1558 AND assigned_to IS NULL) OR assigned_to = 1558) AND (due_at IS NULL AND bucket = 'due_asap') AND (completed_at IS NULL) ORDER BY name ASC, tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE ((user_id = 1558 AND assigned_to IS NULL) OR assigned_to = 1558) AND (due_at >= '2020-05-15 00:00:00' AND due_at < '2020-05-16 00:00:00') AND (completed_at IS NULL) ORDER BY name ASC, tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE ((user_id = 1558 AND assigned_to IS NULL) OR assigned_to = 1558) AND ((due_at IS NULL AND bucket = 'due_later') OR due_at >= '2020-05-25 23:59:59.999999') AND (completed_at IS NULL) ORDER BY name ASC, tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE (user_id = 1561 AND assigned_to IS NOT NULL AND assigned_to != 1561) AND (due_at IS NOT NULL AND due_at < '2020-05-15 00:00:00') AND (completed_at IS NULL) ORDER BY tasks.id DESC, tasks.due_at, tasks.id
SELECT `users`.* FROM `users` WHERE `users`.`id` = 1
SELECT `tasks`.* FROM `tasks` WHERE (user_id = 1561 AND assigned_to IS NOT NULL AND assigned_to != 1561) AND (due_at IS NULL AND bucket = 'due_asap') AND (completed_at IS NULL) ORDER BY tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE (user_id = 1561 AND assigned_to IS NOT NULL AND assigned_to != 1561) AND (due_at >= '2020-05-15 00:00:00' AND due_at < '2020-05-16 00:00:00') AND (completed_at IS NULL) ORDER BY tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE (user_id = 1561 AND assigned_to IS NOT NULL AND assigned_to != 1561) AND ((due_at IS NULL AND bucket = 'due_later') OR due_at >= '2020-05-25 23:59:59.999999') AND (completed_at IS NULL) ORDER BY tasks.id DESC, tasks.due_at, tasks.id
SELECT `tasks`.* FROM `tasks` WHERE ((user_id = 1564 AND assigned_to IS NULL) OR assigned_to = 1564) AND (completed_at >= '2020-05-15 00:00:00' AND completed_at < '2020-05-16 00:00:00') AND (completed_at IS NOT NULL) ORDER BY name ASC, tasks.completed_at DESC
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`name` =  'Hello' AND (`campaigns`.`id` != 42) AND `campaigns`.`user_id` = 404 AND `campaigns`.`deleted_at` IS NULL LIMIT 1
SELECT `tasks`.`id` FROM `tasks` WHERE `tasks`.`asset_id` = 474 AND `tasks`.`asset_type` = 'Lead'
SELECT `account_contacts`.* FROM `account_contacts` WHERE `account_contacts`.`account_id` = 59
SELECT `account_opportunities`.* FROM `account_opportunities` WHERE `account_opportunities`.`account_id` = 59
SELECT `permissions`.* FROM `permissions` WHERE 1=0 AND `permissions`.`asset_id` IS NULL AND `permissions`.`asset_type` = 'Lead'
SELECT  1 AS one FROM `permissions` WHERE `permissions`.`user_id` = 7 AND `permissions`.`group_id` IS NULL AND `permissions`.`asset_id` IS NULL AND `permissions`.`asset_type` = 'Lead' LIMIT 1
SELECT  `emails`.* FROM `emails` WHERE `emails`.`id` = 1 LIMIT 1
SELECT  DISTINCT `contacts`.* FROM `contacts` INNER JOIN `account_contacts` ON `contacts`.`id` = `account_contacts`.`contact_id` WHERE `account_contacts`.`account_id` = 42 ORDER BY updated_at desc LIMIT 20 OFFSET 0
SELECT  DISTINCT `opportunities`.* FROM `opportunities` INNER JOIN `account_opportunities` ON `opportunities`.`id` = `account_opportunities`.`opportunity_id` WHERE `account_opportunities`.`account_id` = 42 ORDER BY opportunities.id DESC, updated_at desc LIMIT 20 OFFSET 0
SELECT DISTINCT `opportunities`.`id` FROM `opportunities` INNER JOIN `contact_opportunities` ON `opportunities`.`id` = `contact_opportunities`.`opportunity_id` WHERE `contact_opportunities`.`contact_id` = 2076 ORDER BY opportunities.id DESC
SELECT  `versions`.* FROM `versions` WHERE (versions.created_at >= '2020-05-13 02:42:45.676538') ORDER BY created_at DESC LIMIT 500
SELECT  DISTINCT `contacts`.* FROM `contacts` INNER JOIN `contact_opportunities` ON `contacts`.`id` = `contact_opportunities`.`contact_id` WHERE `contact_opportunities`.`opportunity_id` = 42 ORDER BY contacts.id DESC, updated_at desc LIMIT 20 OFFSET 0
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`name` IS NULL AND `campaigns`.`user_id` = 2870 AND `campaigns`.`deleted_at` IS NULL LIMIT 1
SELECT  `account_opportunities`.* FROM `account_opportunities` WHERE `account_opportunities`.`opportunity_id` = 1037 LIMIT 1
SELECT COUNT(*) FROM `accounts` WHERE ((`accounts`.`assigned_to` = 238) OR ((`accounts`.`user_id` = 238) OR (`accounts`.`access` = 'Public')))
SELECT COUNT(*) FROM (SELECT DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1359) OR ((`opportunities`.`user_id` = 1359) OR (`opportunities`.`access` = 'Public'))) ORDER BY opportunities.created_at DESC) subquery_for_count
SELECT  DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1359) OR ((`opportunities`.`user_id` = 1359) OR (`opportunities`.`access` = 'Public'))) ORDER BY opportunities.created_at DESC LIMIT 20 OFFSET 820
SELECT COUNT(*) FROM (SELECT DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1359) OR ((`opportunities`.`user_id` = 1359) OR (`opportunities`.`access` = 'Public')))) subquery_for_count
SELECT  `contacts`.* FROM `contacts` WHERE `contacts`.`id` = 42 LIMIT 1
SELECT COUNT(*) FROM `accounts` WHERE ((`accounts`.`assigned_to` = 271) OR ((`accounts`.`user_id` = 271) OR (`accounts`.`access` = 'Public'))) AND `accounts`.`category` = 'affiliate'
SELECT COUNT(*) FROM (SELECT DISTINCT `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 363) OR ((`campaigns`.`user_id` = 363) OR (`campaigns`.`access` = 'Public'))) AND (status IN ('planned','started')) ORDER BY campaigns.created_at DESC) subquery_for_count
SELECT COUNT(*) FROM (SELECT DISTINCT `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 364) OR ((`campaigns`.`user_id` = 364) OR (`campaigns`.`access` = 'Public'))) AND (`campaigns`.`name` LIKE '%again%') ORDER BY campaigns.created_at DESC) subquery_for_count
SELECT COUNT(*) FROM (SELECT DISTINCT `contacts`.* FROM `contacts` WHERE ((`contacts`.`assigned_to` = 547) OR ((`contacts`.`user_id` = 547) OR (`contacts`.`access` = 'Public'))) AND ((`contacts`.`first_name` LIKE '%page_sanford@kuvalisraynor.biz%' OR `contacts`.`last_name` LIKE '%page_sanford@kuvalisraynor.biz%') OR (((`contacts`.`email` LIKE '%page_sanford@kuvalisraynor.biz%' OR `contacts`.`alt_email` LIKE '%page_sanford@kuvalisraynor.biz%') OR `contacts`.`phone` LIKE '%page_sanford@kuvalisraynor.biz%') OR `contacts`.`mobile` LIKE '%page_sanford@kuvalisraynor.biz%')) ORDER BY contacts.created_at DESC) subquery_for_count
SELECT DISTINCT `opportunities`.`id` FROM `opportunities` INNER JOIN `account_opportunities` ON `opportunities`.`id` = `account_opportunities`.`opportunity_id` WHERE `account_opportunities`.`account_id` = 1106 ORDER BY opportunities.id DESC
SELECT DISTINCT `contacts`.`id` FROM `contacts` INNER JOIN `account_contacts` ON `contacts`.`id` = `account_contacts`.`contact_id` WHERE `account_contacts`.`account_id` = 1106
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`name` IS NULL AND `accounts`.`deleted_at` IS NULL LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`assigned_to` IS NULL LIMIT 1
SELECT  1 AS one FROM `accounts` WHERE `accounts`.`user_id` IS NULL LIMIT 1
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`assigned_to` IS NULL LIMIT 1
SELECT  1 AS one FROM `campaigns` WHERE `campaigns`.`user_id` IS NULL LIMIT 1
SELECT  1 AS one FROM `leads` WHERE `leads`.`assigned_to` IS NULL LIMIT 1
SELECT  1 AS one FROM `leads` WHERE `leads`.`user_id` IS NULL LIMIT 1
SELECT  1 AS one FROM `contacts` WHERE `contacts`.`assigned_to` IS NULL LIMIT 1
SELECT  1 AS one FROM `contacts` WHERE `contacts`.`user_id` IS NULL LIMIT 1


SELECT  1 AS one FROM `comments` WHERE `comments`.`user_id` IS NULL LIMIT 1
SELECT  1 AS one FROM `tasks` WHERE `tasks`.`assigned_to` IS NULL LIMIT 1
SELECT  1 AS one FROM `tasks` WHERE `tasks`.`user_id` IS NULL LIMIT 1
SELECT  `leads`.* FROM `leads` WHERE `leads`.`campaign_id` = 42 ORDER BY id DESC LIMIT 20 OFFSET 0
SELECT COUNT(*) FROM `leads` WHERE `leads`.`campaign_id` = 42
SELECT  `opportunities`.* FROM `opportunities` WHERE `opportunities`.`campaign_id` = 42 ORDER BY id DESC, updated_at desc LIMIT 20 OFFSET 0
SELECT  `users`.* FROM `users` WHERE `users`.`username` = 'test_user' LIMIT 1
SELECT COUNT(*) FROM (SELECT DISTINCT `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 1124) OR ((`leads`.`user_id` = 1124) OR (`leads`.`access` = 'Public'))) ORDER BY leads.first_name ASC) subquery_for_count
SELECT  1 AS one FROM `tags` INNER JOIN `taggings` ON `tags`.`id` = `taggings`.`tag_id` WHERE `taggings`.`taggable_id` = 511 AND `taggings`.`taggable_type` = 'Lead' AND `taggings`.`context` = 'tags' LIMIT 1
SELECT  `comments`.* FROM `comments` WHERE `comments`.`id` = 53 LIMIT 1
SELECT COUNT(*) FROM (SELECT DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1503) OR ((`opportunities`.`user_id` = 1503) OR (`opportunities`.`access` = 'Public'))) AND (stage IN ('prospecting')) ORDER BY opportunities.created_at DESC) subquery_for_count
SELECT `fields`.* FROM `fields` WHERE `fields`.`field_group_id` = 1066 AND `fields`.`pair_id` IS NULL ORDER BY `fields`.`position` ASC
SELECT `contact_opportunities`.* FROM `contact_opportunities` WHERE `contact_opportunities`.`opportunity_id` = 1043
SELECT COUNT(*) FROM (SELECT DISTINCT `contacts`.* FROM `contacts` WHERE ((`contacts`.`assigned_to` = 865) OR ((`contacts`.`user_id` = 865) OR (`contacts`.`access` = 'Public'))) ORDER BY contacts.first_name ASC) subquery_for_count
SELECT `leads`.`id` FROM `leads` WHERE `leads`.`campaign_id` = 526 ORDER BY id DESC
SELECT `opportunities`.`id` FROM `opportunities` WHERE `opportunities`.`campaign_id` = 526 ORDER BY id DESC
SELECT `contact_opportunities`.* FROM `contact_opportunities` WHERE `contact_opportunities`.`contact_id` = 2005
SELECT `comments`.* FROM `comments` WHERE `comments`.`commentable_id` = 4 AND `comments`.`commentable_type` = 'Account' ORDER BY created_at DESC
SELECT `permissions`.`asset_type`, `permissions`.`asset_id` FROM `permissions` WHERE `permissions`.`user_id` IS NULL AND `permissions`.`asset_type` IN ('Account', 'Campaign', 'Contact', 'Lead', 'Opportunity')
SELECT COUNT(*) FROM (SELECT DISTINCT `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 541) OR ((`campaigns`.`user_id` = 541) OR (`campaigns`.`access` = 'Public'))) ORDER BY campaigns.name ASC) subquery_for_count
SELECT `users`.* FROM `users` WHERE `users`.`first_name` = 'Billy' AND `users`.`last_name` = 'Bones'
SELECT  `versions`.* FROM `versions` WHERE `versions`.`item_type` = 'Task' AND (versions.created_at >= '2020-05-13 02:42:46.672022') ORDER BY created_at DESC LIMIT 500
SELECT COUNT(*) FROM (SELECT DISTINCT *, amount*probability FROM `opportunities` WHERE ((`opportunities`.`assigned_to` = 1143) OR ((`opportunities`.`user_id` = 1143) OR (`opportunities`.`access` = 'Public'))) AND (`opportunities`.`name` LIKE '%second%') ORDER BY opportunities.created_at DESC) subquery_for_count
SELECT DISTINCT `contacts`.`id` FROM `contacts` INNER JOIN `contact_opportunities` ON `contacts`.`id` = `contact_opportunities`.`contact_id` WHERE `contact_opportunities`.`opportunity_id` = 1128 ORDER BY contacts.id DESC
SELECT COUNT(*) FROM (SELECT DISTINCT `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 879) OR ((`leads`.`user_id` = 879) OR (`leads`.`access` = 'Public'))) AND (((`leads`.`first_name` LIKE '%bill%' OR `leads`.`last_name` LIKE '%bill%') OR `leads`.`company` LIKE '%bill%') OR `leads`.`email` LIKE '%bill%') ORDER BY leads.created_at DESC) subquery_for_count
SELECT  `leads`.* FROM `leads` WHERE (first_name LIKE '%Cindy' AND last_name LIKE '%Cluster') ORDER BY `leads`.`id` ASC LIMIT 1
SELECT  `campaigns`.* FROM `campaigns` WHERE (name LIKE '%Got milk%') ORDER BY `campaigns`.`id` ASC LIMIT 1
SELECT COUNT(DISTINCT `accounts`.`id`) FROM `accounts` WHERE ((`accounts`.`assigned_to` = 289) OR ((`accounts`.`user_id` = 289) OR (`accounts`.`access` = 'Public')))
SELECT  `campaigns`.* FROM `campaigns` WHERE ((`campaigns`.`assigned_to` = 410) OR ((`campaigns`.`user_id` = 410) OR (`campaigns`.`access` = 'Public'))) ORDER BY campaigns.created_at DESC LIMIT 20 OFFSET 0
SELECT `contacts`.`id` FROM `contacts` INNER JOIN `account_contacts` ON `contacts`.`id` = `account_contacts`.`contact_id` WHERE `account_contacts`.`account_id` = 1106
SELECT  `leads`.* FROM `leads` WHERE ((`leads`.`assigned_to` = 980) OR ((`leads`.`user_id` = 980) OR (`leads`.`access` = 'Public'))) ORDER BY leads.created_at DESC LIMIT 20 OFFSET 0
SELECT `groups_users`.`group_id` FROM `groups_users` WHERE `groups_users`.`user_id` = 3056
SELECT COUNT(*) FROM ( SELECT DISTINCT *, amount * probability FROM `opportunities` WHERE ( ( `opportunities`.`assigned_to` = 1143 ) OR ( (`opportunities`.`user_id` = 1143) OR ( `opportunities`.`access` = 'Public' ) ) ) AND ( `opportunities`.`name` LIKE '%second%' ) ) subquery_for_count
