
SELECT COUNT(1) FROM "t_job_log"
SELECT "log_id", "job_id", "bean_name", "method_name", "params", "status", "error", "times", "create_time" FROM "t_job_log" ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0


SELECT * FROM "qrtz_locks" WHERE "sched_name" = 'FEBS_Scheduler' AND "lock_name" = 'TRIGGER_ACCESS' FOR UPDATE
SELECT "job_name" FROM "qrtz_job_details" WHERE "sched_name" = 'FEBS_Scheduler' AND "job_name" = 'TASK_12' AND "job_group" = 'DEFAULT'

SELECT "trigger_name" FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'TASK_12' AND "trigger_group" = 'DEFAULT'
SELECT "trigger_group" FROM "qrtz_paused_trigger_grps" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_group" = 'DEFAULT'


SELECT "trigger_name", "trigger_group" FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "job_name" = 'TASK_12' AND "job_group" = 'DEFAULT'
SELECT * FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'TASK_12' AND "trigger_group" = 'DEFAULT'
SELECT * FROM "qrtz_cron_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'TASK_12' AND "trigger_group" = 'DEFAULT'
SELECT "trigger_state" FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'TASK_12' AND "trigger_group" = 'DEFAULT'


SELECT "dept_id", "parent_id", "dept_name", "order_num", "create_time", "modify_time" FROM "t_dept" WHERE "parent_id" IN ('11')



SELECT "role_id", "role_name", "remark", "create_time", "modify_time" FROM "t_role"
SELECT COUNT(1) FROM "t_log"
SELECT "id", "username", "operation", "time", "method", "params", "ip", "create_time", "location" FROM "t_log" ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "t_log" WHERE "create_time" >= '2019-10-01' AND "create_time" <= '2019-11-21'
SELECT "id", "username", "operation", "time", "method", "params", "ip", "create_time", "location" FROM "t_log" WHERE "create_time" >= '2019-10-01' AND "create_time" <= '2019-11-21' ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0

SELECT "dept_id", "parent_id", "dept_name", "order_num", "create_time", "modify_time" FROM "t_dept" ORDER BY "order_num" ASC NULLS FIRST

SELECT "dept_id", "parent_id", "dept_name", "order_num", "create_time", "modify_time" FROM "t_dept" WHERE "dept_name" = 'e' ORDER BY "order_num" ASC NULLS FIRST
SELECT COUNT(1) FROM "t_login_log"
SELECT "id", "username", "login_time", "location", "ip", "system", "browser" FROM "t_login_log" ORDER BY "login_time" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "t_login_log" WHERE "login_time" >= '2019-10-09' AND "login_time" <= '2019-11-15'
SELECT "id", "username", "login_time", "location", "ip", "system", "browser" FROM "t_login_log" WHERE "login_time" >= '2019-10-09' AND "login_time" <= '2019-11-15' ORDER BY "login_time" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."password", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "u"."theme", "u"."is_tab" AS "istab", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE "u"."username" = 'mrbird' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."password", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar", "u"."theme", "u"."is_tab"

SELECT COUNT(1) FROM "t_eximport"
SELECT "field1", "field2", "field3", "create_time" FROM "t_eximport" LIMIT 10 OFFSET 10
SELECT "job_id", "bean_name", "method_name", "params", "cron_expression", "status", "remark", "create_time" FROM "t_job" WHERE "job_id" = 12
SELECT "r".* FROM "t_role" AS "r" LEFT JOIN "t_user_role" AS "ur" ON ("r"."role_id" = "ur"."role_id") LEFT JOIN "t_user" AS "u" ON ("u"."user_id" = "ur"."user_id") WHERE "u"."username" = 'MrBird'
SELECT "m"."perms" FROM "t_role" AS "r" LEFT JOIN "t_user_role" AS "ur" ON ("r"."role_id" = "ur"."role_id") LEFT JOIN "t_user" AS "u" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role_menu" AS "rm" ON ("rm"."role_id" = "r"."role_id") LEFT JOIN "t_menu" AS "m" ON ("m"."menu_id" = "rm"."menu_id") WHERE "u"."username" = 'MrBird' AND NOT "m"."perms" IS NULL AND "m"."perms" <> ''
SELECT "id", "author", "base_package", "entity_package", "mapper_package", "mapper_xml_package", "service_package", "service_impl_package", "controller_package", "is_trim", "trim_value" FROM "t_generator_config" WHERE "id" = '1'

SELECT COUNT(1) FROM "t_job"
SELECT "job_id", "bean_name", "method_name", "params", "cron_expression", "status", "remark", "create_time" FROM "t_job" ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0





SELECT COUNT(1) FROM (SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar") AS "total"
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar" ORDER BY "userid" ASC NULLS FIRST LIMIT 5 OFFSET 0
SELECT "menu_id", "parent_id", "menu_name", "url", "perms", "icon", "type", "order_num", "create_time", "modify_time" FROM "t_menu" ORDER BY "menu_id" ASC NULLS FIRST, "order_num" ASC NULLS FIRST


SELECT "menu_id", "parent_id", "menu_name", "url", "perms", "icon", "type", "order_num", "create_time", "modify_time" FROM "t_menu" WHERE "parent_id" IN ('178')



SELECT * FROM "qrtz_simple_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'MT_6qmdud2aod6t6' AND "trigger_group" = 'DEFAULT'
SELECT "j"."job_name", "j"."job_group", "j"."is_durable", "j"."job_class_name", "j"."requests_recovery" FROM "qrtz_triggers" AS "t", "qrtz_job_details" AS "j" WHERE "t"."sched_name" = 'FEBS_Scheduler' AND "j"."sched_name" = 'FEBS_Scheduler' AND "t"."trigger_name" = 'MT_6qmdud2aod6t6' AND "t"."trigger_group" = 'DEFAULT' AND "t"."job_name" = "j"."job_name" AND "t"."job_group" = "j"."job_group"


SELECT COUNT("trigger_name") FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "job_name" = 'TASK_12' AND "job_group" = 'DEFAULT'



SELECT "dept_id", "parent_id", "dept_name", "order_num", "create_time", "modify_time" FROM "t_dept" ORDER BY "order_num" ASC NULLS FIRST


SELECT "menu_id", "parent_id", "menu_name", "url", "perms", "icon", "type", "order_num", "create_time", "modify_time" FROM "t_menu" ORDER BY "order_num" ASC NULLS FIRST



SELECT * FROM "qrtz_job_details" WHERE "sched_name" = 'FEBS_Scheduler' AND "job_name" = 'TASK_12' AND "job_group" = 'DEFAULT'







SELECT COUNT(1) FROM "t_login_log" WHERE CAST(AGE(CAST("login_time" AS TIMESTAMP), CAST(NOW() AS TIMESTAMP)) AS BIGINT) = 0
SELECT COUNT(DISTINCT ("ip")) FROM "t_login_log" WHERE CAST(AGE(CAST("login_time" AS TIMESTAMP), CAST(NOW() AS TIMESTAMP)) AS BIGINT) = 0
SELECT TO_CHAR("l"."login_time", 'MM-DD') AS "days", COUNT(1) AS "count" FROM (SELECT * FROM "t_login_log" WHERE CURDATE() - INTERVAL '10 DAY' <= CAST("login_time" AS DATE)) AS "l" WHERE 1 = 1 GROUP BY "days"
SELECT TO_CHAR("l"."login_time", 'MM-DD') AS "days", COUNT(1) AS "count" FROM (SELECT * FROM "t_login_log" WHERE CURDATE() - INTERVAL '10 DAY' <= CAST("login_time" AS DATE)) AS "l" WHERE 1 = 1 AND "l"."username" = 'MrBird' GROUP BY "days"
SELECT "id", "author", "base_package", "entity_package", "mapper_package", "mapper_xml_package", "service_package", "service_impl_package", "controller_package", "is_trim", "trim_value" FROM "t_generator_config"
SELECT "trigger_state", "next_fire_time", "job_name", "job_group" FROM "qrtz_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "trigger_name" = 'TASK_12' AND "trigger_group" = 'DEFAULT'
SELECT * FROM "qrtz_fired_triggers" WHERE "sched_name" = 'FEBS_Scheduler' AND "job_name" = 'TASK_12' AND "job_group" = 'DEFAULT'

SELECT "dept_id", "parent_id", "dept_name", "order_num", "create_time", "modify_time" FROM "t_dept"
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar" ORDER BY "userid" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM (SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."username" = 'Jana' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar") AS "total"
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."username" = 'Jana' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar" ORDER BY "userid" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM (SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."ssex" = '1' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar") AS "total"
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."ssex" = '1' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar" ORDER BY "userid" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM (SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."mobile" = '17711111111' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar") AS "total"
SELECT "u"."user_id" AS "userid", "u"."username", "u"."email", "u"."mobile", "u"."status", "u"."create_time" AS "createtime", "u"."ssex" AS "sex", "u"."dept_id" AS "deptid", "u"."last_login_time" AS "lastlogintime", "u"."modify_time" AS "modifytime", "u"."description", "u"."avatar", "d"."dept_name" AS "deptname", STRING_AGG("r"."role_id", ',') AS "roleid", STRING_AGG("r"."role_name", ',') AS "rolename" FROM "t_user" AS "u" LEFT JOIN "t_dept" AS "d" ON ("u"."dept_id" = "d"."dept_id") LEFT JOIN "t_user_role" AS "ur" ON ("u"."user_id" = "ur"."user_id") LEFT JOIN "t_role" AS "r" ON "r"."role_id" = "ur"."role_id" WHERE 1 = 1 AND "u"."mobile" = '17711111111' GROUP BY "u"."username", "u"."user_id", "u"."email", "u"."mobile", "u"."status", "u"."create_time", "u"."ssex", "u"."dept_id", "u"."last_login_time", "u"."modify_time", "u"."description", "u"."avatar" ORDER BY "userid" ASC NULLS FIRST LIMIT 5 OFFSET 0

SELECT COUNT(1) FROM (SELECT "r"."role_id" AS "roleid", "r"."role_name" AS "rolename", "r"."remark", "r"."create_time" AS "createtime", "r"."modify_time" AS "modifytime", STRING_AGG("rm"."menu_id", ',') AS "menuids" FROM "t_role" AS "r" LEFT JOIN "t_role_menu" AS "rm" ON ("r"."role_id" = "rm"."role_id") WHERE 1 = 1 GROUP BY "r"."role_id") AS "total"
SELECT "r"."role_id" AS "roleid", "r"."role_name" AS "rolename", "r"."remark", "r"."create_time" AS "createtime", "r"."modify_time" AS "modifytime", STRING_AGG("rm"."menu_id", ',') AS "menuids" FROM "t_role" AS "r" LEFT JOIN "t_role_menu" AS "rm" ON ("r"."role_id" = "rm"."role_id") WHERE 1 = 1 GROUP BY "r"."role_id" ORDER BY "createtime" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "r"."role_id" AS "roleid", "r"."role_name" AS "rolename", "r"."remark", "r"."create_time" AS "createtime", "r"."modify_time" AS "modifytime", STRING_AGG("rm"."menu_id", ',') AS "menuids" FROM "t_role" AS "r" LEFT JOIN "t_role_menu" AS "rm" ON ("r"."role_id" = "rm"."role_id") WHERE 1 = 1 GROUP BY "r"."role_id" ORDER BY "createtime" DESC NULLS LAST LIMIT 5 OFFSET 0
SELECT "m".* FROM "t_menu" AS "m" WHERE "m"."type" <> 1 AND "m"."menu_id" IN (SELECT DISTINCT "rm"."menu_id" FROM "t_role_menu" AS "rm" LEFT JOIN "t_role" AS "r" ON ("rm"."role_id" = "r"."role_id") LEFT JOIN "t_user_role" AS "ur" ON ("ur"."role_id" = "r"."role_id") LEFT JOIN "t_user" AS "u" ON ("u"."user_id" = "ur"."user_id") WHERE "u"."username" = 'MrBird') ORDER BY "m"."order_num" NULLS FIRST
SELECT COUNT(DISTINCT "ip") FROM "t_login_log" WHERE "login_time" BETWEEN CURDATE() AND CURDATE() + INTERVAL '1 DAY' - INTERVAL '1 SECOND'
SELECT COUNT(1) FROM "t_login_log" WHERE "login_time" BETWEEN CURDATE() AND CURDATE() + INTERVAL '1 DAY' - INTERVAL '1 SECOND'
