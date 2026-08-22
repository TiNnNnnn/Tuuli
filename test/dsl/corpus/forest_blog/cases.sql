SELECT "menu_id", "menu_name", "menu_url", "menu_level", "menu_icon", "menu_order" FROM "menu" ORDER BY "menu_order" DESC NULLS LAST, "menu_id" ASC NULLS FIRST
SELECT "category_id", "category_pid", "category_name", "category_description", "category_order", "category_icon" FROM "category" ORDER BY "category_order" DESC NULLS LAST, "category_id" ASC NULLS FIRST
SELECT COUNT(*) FROM "article" WHERE "article_status" = 1
SELECT SUM("article_comment_count") FROM "article" WHERE "article_status" = 1
SELECT COUNT(*) FROM "category"
SELECT COUNT(*) FROM "tag"
SELECT COUNT(*) FROM "link" WHERE "link_status" = 1
SELECT SUM("article_view_count") FROM "article" WHERE "article_status" = 1
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 AND "article_update_time" = (SELECT MAX("article_update_time") FROM "article")
SELECT * FROM "options" LIMIT 1
SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" WHERE "comment_id" = 31




SELECT "notice_id", "notice_title", "notice_content", "notice_create_time", "notice_update_time", "notice_status", "notice_order" FROM "notice" WHERE "notice_id" = 3


SELECT "user_id", "user_name", "user_pass", "user_nickname", "user_email", "user_url", "user_avatar", "user_last_login_ip", "user_register_time", "user_last_login_time", "user_status" FROM "user" WHERE "user_id" = 1


SELECT "link_id", "link_url", "link_name", "link_image", "link_description", "link_owner_nickname", "link_owner_contact", "link_update_time", "link_create_time", "link_order", "link_status" FROM "link" ORDER BY "link_status" ASC NULLS FIRST, "link_order" DESC NULLS LAST, "link_id" ASC NULLS FIRST

SELECT COUNT(*) FROM "article_category_ref" WHERE "category_id" = 100000007


SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 ORDER BY "article_comment_count" DESC NULLS LAST, "article_order" DESC NULLS LAST, "article_id" DESC NULLS LAST LIMIT 8
SELECT "user_id", "user_name", "user_pass", "user_nickname", "user_email", "user_url", "user_avatar", "user_last_login_ip", "user_register_time", "user_last_login_time", "user_status" FROM "user" WHERE "user_name" = 'admin' OR "user_email" = 'admin' AND "user_status" > 0 LIMIT 1

SELECT "menu_id", "menu_name", "menu_url", "menu_level", "menu_icon", "menu_order" FROM "menu" WHERE "menu_id" = 3

SELECT "page_id", "page_key", "page_title", "page_content", "page_create_time", "page_update_time", "page_view_count", "page_comment_count", "page_status" FROM "page" WHERE "page_status" = 1 AND "page_key" = 'aboutSite'
SELECT "article_id", "article_user_id", "article_title", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" ORDER BY "article_id" DESC NULLS LAST LIMIT 5
SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" WHERE "comment_role" = 0 ORDER BY "comment_id" DESC NULLS LAST LIMIT 5
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 AND "article_id" = 6
SELECT "user_id", "user_name", "user_pass", "user_nickname", "user_email", "user_url", "user_avatar", "user_last_login_ip", "user_register_time", "user_last_login_time", "user_status" FROM "user" ORDER BY "user_status" ASC NULLS FIRST
SELECT COUNT(*) FROM "article" WHERE "article_user_id" = 1 AND "article_status" = 1





SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_id" = 7
SELECT "category"."category_id", "category"."category_pid", "category"."category_name" FROM "category", "article_category_ref" WHERE "article_category_ref"."article_id" = 7 AND "article_category_ref"."category_id" = "category"."category_id" ORDER BY "category"."category_pid" ASC NULLS FIRST
SELECT "tag".* FROM "tag", "article_tag_ref" WHERE "article_tag_ref"."article_id" = 7 AND "article_tag_ref"."tag_id" = "tag"."tag_id"
SELECT "tag_id", "tag_name", "tag_description" FROM "tag"
SELECT "page_id", "page_key", "page_title", "page_content", "page_create_time", "page_update_time", "page_view_count", "page_comment_count", "page_status" FROM "page" WHERE "page_key" = 'aboutSite'

SELECT COUNT(*) FROM "article_tag_ref" WHERE "tag_id" = 17



SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" WHERE "comment_pid" = 33


SELECT "page_id", "page_key", "page_title", "page_content", "page_create_time", "page_update_time", "page_view_count", "page_comment_count", "page_status" FROM "page" WHERE "page_id" = 5



SELECT "article_id", "article_user_id", "article_title", "article_create_time" FROM "article" WHERE "article_status" = 1 ORDER BY "article_id" DESC NULLS LAST
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 ORDER BY "article_comment_count" DESC NULLS LAST, "article_order" DESC NULLS LAST, "article_id" DESC NULLS LAST LIMIT 10
SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" WHERE "comment_article_id" = 33 ORDER BY "comment_id" ASC NULLS FIRST
SELECT "category_id" FROM "article_category_ref" WHERE "article_id" = 33
SELECT "article"."article_id", "article"."article_user_id", "article"."article_title", "article"."article_view_count", "article"."article_comment_count", "article"."article_like_count", "article"."article_create_time", "article"."article_update_time", "article"."article_is_comment", "article"."article_status", "article"."article_order", "article"."article_summary" FROM "article", "article_category_ref" WHERE "article"."article_status" = 1 AND "article"."article_id" = "article_category_ref"."article_id" AND "article_category_ref"."category_id" IN (10, 13) LIMIT 5
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 ORDER BY "article_view_count" DESC NULLS LAST, "article_order" DESC NULLS LAST, "article_id" DESC NULLS LAST LIMIT 5
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_id" > 33 AND "article_status" = 1 ORDER BY "article_id" NULLS FIRST LIMIT 1
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_id" < 33 AND "article_status" = 1 ORDER BY "article_id" NULLS FIRST LIMIT 1
SELECT "article_id", "article_user_id", "article_title", "article_content", "article_summary", "article_view_count", "article_comment_count", "article_like_count", "article_create_time", "article_update_time", "article_is_comment", "article_status", "article_order" FROM "article" WHERE "article_status" = 1 ORDER BY RANDOM() NULLS FIRST LIMIT 8
SELECT "user_id", "user_name", "user_pass", "user_nickname", "user_email", "user_url", "user_avatar", "user_last_login_ip", "user_register_time", "user_last_login_time", "user_status" FROM "user" WHERE "user_name" = 'zhouz' LIMIT 1
SELECT COUNT(0) FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_title" LIKE CONCAT(CONCAT('%', 'MySQL'), '%') AND 1 = 1
SELECT "article".* FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_title" LIKE CONCAT(CONCAT('%', 'MySQL'), '%') AND 1 = 1 ORDER BY "article"."article_order" DESC NULLS LAST, "article"."article_id" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" WHERE "comment_role" = 0 ORDER BY "comment_id" DESC NULLS LAST LIMIT 10


SELECT "tag_id", "tag_name", "tag_description" FROM "tag" WHERE "tag_id" = 38

SELECT COUNT(0) FROM "article" WHERE "article"."article_status" = 1 AND 1 = 1
SELECT "article".* FROM "article" WHERE "article"."article_status" = 1 AND 1 = 1 ORDER BY "article"."article_order" DESC NULLS LAST, "article"."article_id" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "notice_id", "notice_title", "notice_content", "notice_create_time", "notice_update_time", "notice_status", "notice_order" FROM "notice" WHERE "notice_status" = 1 ORDER BY "notice_status" ASC NULLS FIRST, "notice_order" DESC NULLS LAST, "notice_id" ASC NULLS FIRST
SELECT "link_id", "link_url", "link_name", "link_image", "link_description", "link_owner_nickname", "link_owner_contact", "link_update_time", "link_create_time", "link_order", "link_status" FROM "link" WHERE "link_status" = 1 ORDER BY "link_status" ASC NULLS FIRST, "link_order" DESC NULLS LAST, "link_id" ASC NULLS FIRST

SELECT COUNT(0) FROM "comment"
SELECT "comment_id", "comment_pid", "comment_pname", "comment_article_id", "comment_author_name", "comment_author_email", "comment_author_url", "comment_author_avatar", "comment_content", "comment_agent", "comment_ip", "comment_create_time", "comment_role" FROM "comment" ORDER BY "comment_id" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "user_id", "user_name", "user_pass", "user_nickname", "user_email", "user_url", "user_avatar", "user_last_login_ip", "user_register_time", "user_last_login_time", "user_status" FROM "user" WHERE "user_email" = 'zhouz@zhouz.com' LIMIT 1

SELECT "link_id", "link_url", "link_name", "link_image", "link_description", "link_owner_nickname", "link_owner_contact", "link_update_time", "link_create_time", "link_order", "link_status" FROM "link" WHERE "link_id" = 6


SELECT "notice_id", "notice_title", "notice_content", "notice_create_time", "notice_update_time", "notice_status", "notice_order" FROM "notice" ORDER BY "notice_status" ASC NULLS FIRST, "notice_order" DESC NULLS LAST, "notice_id" ASC NULLS FIRST



SELECT COUNT(0) FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_id" IN (SELECT "article_tag_ref"."article_id" FROM "article_tag_ref" WHERE "article_tag_ref"."tag_id" = 1) AND 1 = 1
SELECT "article".* FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_id" IN (SELECT "article_tag_ref"."article_id" FROM "article_tag_ref" WHERE "article_tag_ref"."tag_id" = 1) AND 1 = 1 ORDER BY "article"."article_order" DESC NULLS LAST, "article"."article_id" DESC NULLS LAST LIMIT 10 OFFSET 0

SELECT COUNT(0) FROM "article" WHERE 1 = 1
SELECT "article".* FROM "article" WHERE 1 = 1 ORDER BY "article"."article_order" DESC NULLS LAST, "article"."article_id" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "category_id", "category_pid", "category_name", "category_description", "category_order", "category_icon" FROM "category" WHERE "category"."category_id" = 1

SELECT "page_id", "page_key", "page_title", "page_content", "page_create_time", "page_update_time", "page_view_count", "page_comment_count", "page_status" FROM "page"
SELECT COUNT(0) FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_id" IN (SELECT "article_category_ref"."article_id" FROM "article_category_ref" WHERE "article_category_ref"."category_id" = 1) AND 1 = 1
SELECT "article".* FROM "article" WHERE "article"."article_status" = 1 AND "article"."article_id" IN (SELECT "article_category_ref"."article_id" FROM "article_category_ref" WHERE "article_category_ref"."category_id" = 1) AND 1 = 1 ORDER BY "article"."article_order" DESC NULLS LAST, "article"."article_id" DESC NULLS LAST LIMIT 10 OFFSET 0
