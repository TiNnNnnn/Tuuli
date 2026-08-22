SELECT "id", "key", "value", "description", "pid", "type", "option", "reboot" FROM "system_config"
SELECT "u".*, "r"."name" AS "rolename" FROM "admin_user" AS "u" LEFT JOIN "role" AS "r" ON "u"."role_id" = "r"."id"
SELECT "id", "username", "password", "in_time", "role_id" FROM "admin_user" WHERE "username" = 'admin'
SELECT "id", "name" FROM "role" WHERE "id" = 1
SELECT "role_id", "permission_id" FROM "role_permission" WHERE "role_id" = 1
SELECT "id", "name", "value", "pid" FROM "permission" WHERE "id" IN (11, 12, 13, 14, 15, 16, 45, 46, 48, 49, 17, 18, 19, 20, 21, 22, 23, 24, 43, 25, 26, 27, 28, 29, 30, 31, 32, 44, 38, 39, 40, 41, 35, 36, 37, 42, 51, 52, 53, 54, 55)
SELECT "id", "key", "value", "description", "pid", "type", "option", "reboot" FROM "system_config" WHERE "key" = 'redis_host'
SELECT "id", "title", "content", "in_time", "modify_time", "user_id", "comment_count", "collect_count", "view", "top", "good", "up_ids" FROM "topic" WHERE "id" = 2
SELECT "id", "username", "telegram_name", "avatar", "password", "email", "mobile", "website", "bio", "score", "in_time", "token", "email_notification", "active" FROM "user" WHERE "token" = '0234c153-fc73-4901-a4e5-edc7a55a603d'
SELECT "id", "title", "content", "in_time", "modify_time", "user_id", "comment_count", "collect_count", "view", "top", "good", "up_ids" FROM "topic" WHERE "title" = 'd'


SELECT "id", "topic_id", "user_id", "content", "in_time", "comment_id", "up_ids" FROM "comment" WHERE "id" = 1


SELECT COUNT(1) FROM "topic" AS "t" WHERE (CAST(EXTRACT(epoch FROM CAST(CAST("t"."in_time" AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1) = (CAST(EXTRACT(epoch FROM CAST(CAST(NOW() AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1)
SELECT COUNT(1) FROM "tag" WHERE (CAST(EXTRACT(epoch FROM CAST(CAST("in_time" AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1) = (CAST(EXTRACT(epoch FROM CAST(CAST(NOW() AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1)
SELECT COUNT(1) FROM "comment" WHERE (CAST(EXTRACT(epoch FROM CAST(CAST("in_time" AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1) = (CAST(EXTRACT(epoch FROM CAST(CAST(NOW() AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1)
SELECT COUNT(1) FROM "user" WHERE (CAST(EXTRACT(epoch FROM CAST(CAST("in_time" AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1) = (CAST(EXTRACT(epoch FROM CAST(CAST(NOW() AS DATE) AS TIMESTAMP) - CAST(CAST('0000-01-01' AS DATE) AS TIMESTAMP)) / 86400 AS BIGINT) + 1)
SELECT "id", "username", "telegram_name", "avatar", "password", "email", "mobile", "website", "bio", "score", "in_time", "token", "email_notification", "active" FROM "user" WHERE "username" = 'stephen3'
SELECT "id", "oauth_id", "type", "login", "access_token", "in_time", "bio", "email", "user_id", "refresh_token", "union_id", "expires_in" FROM "oauth_user" WHERE "user_id" = 3
SELECT COUNT(1) FROM "collect" WHERE "user_id" = 3

SELECT COUNT(1) FROM "user"
SELECT "id", "username", "telegram_name", "avatar", "password", "email", "mobile", "website", "bio", "score", "in_time", "token", "email_notification", "active" FROM "user" ORDER BY "in_time" DESC NULLS LAST LIMIT 20 OFFSET 0
SELECT COUNT(1) FROM "user" WHERE "username" = 'd'
SELECT "id", "name" FROM "role"

SELECT "id", "username", "telegram_name", "avatar", "password", "email", "mobile", "website", "bio", "score", "in_time", "token", "email_notification", "active" FROM "user" WHERE "id" = 1

SELECT COUNT(1) FROM "topic" AS "t" LEFT JOIN "user" AS "u" ON "t"."user_id" = "u"."id"
SELECT "t".*, "u"."username" FROM "topic" AS "t" LEFT JOIN "user" AS "u" ON "t"."user_id" = "u"."id" ORDER BY "t"."in_time" DESC NULLS LAST LIMIT 20 OFFSET 0
SELECT COUNT(1) FROM "topic" AS "t" LEFT JOIN "user" AS "u" ON "t"."user_id" = "u"."id" WHERE "t"."in_time" BETWEEN '2019-10-11' AND '2019-10-26'
SELECT "t".*, "u"."username" FROM "topic" AS "t" LEFT JOIN "user" AS "u" ON "t"."user_id" = "u"."id" WHERE "t"."in_time" BETWEEN '2019-10-11' AND '2019-10-26' ORDER BY "t"."in_time" DESC NULLS LAST LIMIT 20 OFFSET 0
SELECT "tag_id", "topic_id" FROM "topic_tag" WHERE "topic_id" = 3
SELECT "topic_id", "user_id", "in_time" FROM "collect" WHERE "topic_id" = 3
SELECT "topic_id", "user_id", "in_time" FROM "collect" WHERE "topic_id" = 3 AND "user_id" = 1





SELECT COUNT(1) FROM "tag"
SELECT "id", "name", "description", "icon", "topic_count", "in_time" FROM "tag" ORDER BY "topic_count" DESC NULLS LAST

SELECT COUNT(1) FROM "sensitive_word"
SELECT "id", "word" FROM "sensitive_word"
SELECT COUNT(1) FROM "sensitive_word" WHERE "word" = 'fuck'
SELECT "id", "word" FROM "sensitive_word" WHERE "word" = 'fuck'
SELECT "id", "name", "value", "pid" FROM "permission" WHERE "pid" = 0
SELECT COUNT(1) FROM "comment" AS "c" LEFT JOIN "topic" AS "t" ON "c"."topic_id" = "t"."id" LEFT JOIN "user" AS "u" ON "u"."id" = "c"."user_id"
SELECT "c".*, "t"."title", "t"."id" AS "topicid", "u"."username" FROM "comment" AS "c" LEFT JOIN "topic" AS "t" ON "c"."topic_id" = "t"."id" LEFT JOIN "user" AS "u" ON "u"."id" = "c"."user_id" ORDER BY "c"."in_time" DESC NULLS LAST LIMIT 20 OFFSET 0
SELECT COUNT(1) FROM "comment" AS "c" LEFT JOIN "topic" AS "t" ON "c"."topic_id" = "t"."id" LEFT JOIN "user" AS "u" ON "u"."id" = "c"."user_id" WHERE "c"."in_time" BETWEEN '2019-10-01' AND '2019-10-31'
SELECT "c".*, "t"."title", "t"."id" AS "topicid", "u"."username" FROM "comment" AS "c" LEFT JOIN "topic" AS "t" ON "c"."topic_id" = "t"."id" LEFT JOIN "user" AS "u" ON "u"."id" = "c"."user_id" WHERE "c"."in_time" BETWEEN '2019-10-01' AND '2019-10-31' ORDER BY "c"."in_time" DESC NULLS LAST LIMIT 20 OFFSET 0
SELECT "id", "username", "telegram_name", "avatar", "password", "email", "mobile", "website", "bio", "score", "in_time", "token", "email_notification", "active" FROM "user" WHERE "email" = '321@qq.com'

SELECT "id", "user_id", "code", "in_time", "expire_time", "email", "mobile", "used" FROM "code" WHERE "email" = '321@qq.com' AND "user_id" = 3 AND "used" = 0 AND "expire_time" > '2019-10-21 19:50:16.271'
SELECT "id", "user_id", "code", "in_time", "expire_time", "email", "mobile", "used" FROM "code" WHERE "code" = '378640'






SELECT COUNT(1) FROM "notification" WHERE "target_user_id" = 3 AND "read" = 0
SELECT COUNT(1) FROM "user" WHERE "in_time" BETWEEN CURDATE() AND CURDATE() + INTERVAL '1 DAY' - INTERVAL '1 SECOND'
