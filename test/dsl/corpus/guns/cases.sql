SELECT "url" FROM "sys_relation" AS "rel" INNER JOIN "sys_menu" AS "m" ON "rel"."menu_id" = "m"."menu_id" WHERE "rel"."role_id" = 1
SELECT "role_id", "pid", "name", "description", "sort", "version", "create_time", "update_time", "create_user", "update_user" FROM "sys_role" WHERE "role_id" = 1
SELECT "dept_id", "pid", "pids", "simple_name", "full_name", "description", "version", "sort", "create_time", "update_time", "create_user", "update_user" FROM "sys_dept" WHERE "dept_id" = 25

SELECT "user_id", "avatar", "account", "password", "salt", "name", "birthday", "sex", "email", "phone", "role_id", "dept_id", "status", "create_time", "create_user", "update_time", "update_user", "version" FROM "sys_user" WHERE "user_id" = 1188369796975165441

SELECT COUNT(1) FROM "sys_notice"
SELECT "notice_id" AS "noticeid", "title" AS "title", "content" AS "content", "create_time" AS "createtime", "create_user" AS "createuser", "update_time" AS "updatetime", "update_user" AS "updateuser" FROM "sys_notice" ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_notice" WHERE "title" LIKE CONCAT('%', 'd', '%') OR "content" LIKE CONCAT('%', 'd', '%')
SELECT "notice_id" AS "noticeid", "title" AS "title", "content" AS "content", "create_time" AS "createtime", "create_user" AS "createuser", "update_time" AS "updatetime", "update_user" AS "updateuser" FROM "sys_notice" WHERE "title" LIKE CONCAT('%', 'd', '%') OR "content" LIKE CONCAT('%', 'd', '%') ORDER BY "create_time" DESC NULLS LAST LIMIT 10 OFFSET 0
SELECT "notice_id" AS "noticeid", "title" AS "title", "content" AS "content", "create_time" AS "createtime", "create_user" AS "createuser", "update_time" AS "updatetime", "update_user" AS "updateuser" FROM "sys_notice" WHERE "title" LIKE CONCAT('%', 'd', '%') OR "content" LIKE CONCAT('%', 'd', '%') ORDER BY "create_time" DESC NULLS LAST

SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE "dict_type_id" = 1188371218487705601
SELECT COUNT(1) FROM "sys_dict_type" WHERE ("code" = 'NOUSE' OR "name" = 'd') AND ("dict_type_id" <> 1188371218487705601)

SELECT "user_id" AS "userid", "avatar" AS "avatar", "account" AS "account", "salt" AS "salt", "password" AS "password", "name" AS "name", "birthday" AS "birthday", "sex" AS "sex", "email" AS "email", "phone" AS "phone", "role_id" AS "roleid", "dept_id" AS "deptid", "status" AS "status", "create_time" AS "createtime", "create_user" AS "createuser", "update_time" AS "updatetime", "update_user" AS "updateuser", "version" AS "version" FROM "sys_user" WHERE "account" = 'test1' AND "status" <> 'DELETED'

SELECT "notice_id", "title", "content", "create_time", "create_user", "update_time", "update_user" FROM "sys_notice" WHERE "notice_id" = 1188376366811316226



SELECT "menu_id", "code", "pcode", "pcodes", "name", "icon", "url", "sort", "levels", "menu_flag", "description", "status", "new_page_flag", "open_flag", "create_time", "update_time", "create_user", "update_user" FROM "sys_menu" WHERE "menu_id" = 1188371829954314242


SELECT "menu_id" AS "menuid", "code" AS "code", "pcode" AS "pcode", "pcodes" AS "pcodes", "name" AS "name", "icon" AS "icon", "url" AS "url", "sort" AS "sort", "levels" AS "levels", "menu_flag" AS "menuflag", "description" AS "description", "status" AS "status", "new_page_flag" AS "newpageflag", "open_flag" AS "openflag", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_menu" WHERE 1 = 1 AND "pcodes" LIKE CONCAT('%$[', '142', '$]%') ESCAPE '$'
SELECT "r"."role_id" AS "id", "r"."pid", "r"."name", (CASE WHEN ("r"."pid" = 0 OR "r"."pid" IS NULL) THEN 'true' ELSE 'false' END) AS "open", (CASE WHEN ("r1"."role_id" = 0 OR "r1"."role_id" IS NULL) THEN 'false' ELSE 'true' END) AS "checked" FROM "sys_role" AS "r" LEFT JOIN (SELECT "role_id" FROM "sys_role" WHERE "role_id" IN (1)) AS "r1" ON "r"."role_id" = "r1"."role_id" ORDER BY "pid" NULLS FIRST, "sort" ASC NULLS FIRST
SELECT COUNT(1) FROM "sys_operation_log" WHERE 1 = 1
SELECT "operation_log_id" AS "operationlogid", "log_type" AS "logtype", "log_name" AS "logname", "user_id" AS "userid", "class_name" AS "classname", "method" AS "method", "create_time" AS "createtime", "succeed" AS "succeed", "message" AS "message" FROM "sys_operation_log" WHERE 1 = 1 LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_operation_log" WHERE 1 = 1 AND "log_type" LIKE CONCAT('%', 'd', '%')
SELECT "operation_log_id" AS "operationlogid", "log_type" AS "logtype", "log_name" AS "logname", "user_id" AS "userid", "class_name" AS "classname", "method" AS "method", "create_time" AS "createtime", "succeed" AS "succeed", "message" AS "message" FROM "sys_operation_log" WHERE 1 = 1 AND "log_type" LIKE CONCAT('%', 'd', '%') LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_operation_log" WHERE 1 = 1 AND ("create_time" BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59')) AND "log_type" LIKE CONCAT('%', 'd', '%')
SELECT "operation_log_id" AS "operationlogid", "log_type" AS "logtype", "log_name" AS "logname", "user_id" AS "userid", "class_name" AS "classname", "method" AS "method", "create_time" AS "createtime", "succeed" AS "succeed", "message" AS "message" FROM "sys_operation_log" WHERE 1 = 1 AND ("create_time" BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59')) AND "log_type" LIKE CONCAT('%', 'd', '%') LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_user" WHERE "status" <> 'DELETED'
SELECT "user_id" AS "userid", "avatar" AS "avatar", "account" AS "account", "salt" AS "salt", "name" AS "name", "birthday" AS "birthday", "sex" AS "sex", "email" AS "email", "phone" AS "phone", "role_id" AS "roleid", "dept_id" AS "deptid", "status" AS "status", "create_time" AS "createtime", "create_user" AS "createuser", "update_time" AS "updatetime", "update_user" AS "updateuser", "version" AS "version" FROM "sys_user" WHERE "status" <> 'DELETED' LIMIT 10 OFFSET 0
SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE "name" = 'd'
SELECT "dict_id", "dict_type_id", "code", "name", "parent_id", "parent_ids", "status", "sort", "description", "create_time", "update_time", "create_user", "update_user" FROM "sys_dict" WHERE "dict_type_id" = 1106120208097067009
SELECT "menu_id", "code", "pcode", "pcodes", "name", "icon", "url", "sort", "levels", "menu_flag", "description", "status", "new_page_flag", "open_flag", "create_time", "update_time", "create_user", "update_user" FROM "sys_menu" WHERE "code" = '1'

SELECT COUNT(1) FROM "sys_dept" WHERE 1 = 1
SELECT "dept_id" AS "deptid", "pid" AS "pid", "pids" AS "pids", "simple_name" AS "simplename", "full_name" AS "fullname", "description" AS "description", "version" AS "version", "sort" AS "sort", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_dept" WHERE 1 = 1 ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_dept" WHERE 1 = 1 AND ("dept_id" = 24 OR "dept_id" IN (SELECT "dept_id" FROM "sys_dept" WHERE "pids" LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$'))
SELECT "dept_id" AS "deptid", "pid" AS "pid", "pids" AS "pids", "simple_name" AS "simplename", "full_name" AS "fullname", "description" AS "description", "version" AS "version", "sort" AS "sort", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_dept" WHERE 1 = 1 AND ("dept_id" = 24 OR "dept_id" IN (SELECT "dept_id" FROM "sys_dept" WHERE "pids" LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$')) ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_dept" WHERE 1 = 1 AND "simple_name" LIKE CONCAT('%', 'd', '%') OR "full_name" LIKE CONCAT('%', 'd', '%') AND ("dept_id" = 24 OR "dept_id" IN (SELECT "dept_id" FROM "sys_dept" WHERE "pids" LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$'))
SELECT "dept_id" AS "deptid", "pid" AS "pid", "pids" AS "pids", "simple_name" AS "simplename", "full_name" AS "fullname", "description" AS "description", "version" AS "version", "sort" AS "sort", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_dept" WHERE 1 = 1 AND "simple_name" LIKE CONCAT('%', 'd', '%') OR "full_name" LIKE CONCAT('%', 'd', '%') AND ("dept_id" = 24 OR "dept_id" IN (SELECT "dept_id" FROM "sys_dept" WHERE "pids" LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$')) ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT "menu_id" FROM "sys_relation" WHERE "role_id" = 1188370535139115009
SELECT "m1"."menu_id" AS "id", (CASE WHEN ("m2"."menu_id" = 0 OR "m2"."menu_id" IS NULL) THEN 0 ELSE "m2"."menu_id" END) AS "pid", "m1"."name" AS "name", (CASE WHEN ("m2"."menu_id" = 0 OR "m2"."menu_id" IS NULL) THEN 'true' ELSE 'false' END) AS "open" FROM "sys_menu" AS "m1" LEFT JOIN "sys_menu" AS "m2" ON "m1"."pcode" = "m2"."code" ORDER BY "m1"."menu_id" ASC NULLS FIRST
SELECT "dept_id" AS "id", "pid" AS "pid", "simple_name" AS "name", (CASE WHEN ("pid" = 0 OR "pid" IS NULL) THEN 'true' ELSE 'false' END) AS "open" FROM "sys_dept"
SELECT COUNT(1) FROM "sys_login_log" WHERE 1 = 1
SELECT "login_log_id" AS "loginlogid", "log_name" AS "logname", "user_id" AS "userid", "create_time" AS "createtime", "succeed" AS "succeed", "message" AS "message", "ip_address" AS "ipaddress" FROM "sys_login_log" WHERE 1 = 1 LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_login_log" WHERE 1 = 1 AND ("create_time" BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59'))
SELECT "login_log_id" AS "loginlogid", "log_name" AS "logname", "user_id" AS "userid", "create_time" AS "createtime", "succeed" AS "succeed", "message" AS "message", "ip_address" AS "ipaddress" FROM "sys_login_log" WHERE 1 = 1 AND ("create_time" BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59')) LIMIT 10 OFFSET 0





SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE "code" = 'NOUSE' OR "name" = 'e'


SELECT "m1"."menu_id" AS "id", "m1"."icon" AS "icon", (CASE WHEN ("m2"."menu_id" = 0 OR "m2"."menu_id" IS NULL) THEN 0 ELSE "m2"."menu_id" END) AS "parentid", "m1"."name" AS "name", "m1"."url" AS "url", "m1"."levels" AS "levels", "m1"."menu_flag" AS "ismenu", "m1"."sort" AS "num" FROM "sys_menu" AS "m1" LEFT JOIN "sys_menu" AS "m2" ON "m1"."pcode" = "m2"."code" INNER JOIN (SELECT "menu_id" FROM "sys_menu" WHERE "menu_id" IN (SELECT "menu_id" FROM "sys_relation" AS "rela" WHERE "rela"."role_id" IN (1))) AS "m3" ON "m1"."menu_id" = "m3"."menu_id" WHERE "m1"."menu_flag" = 'Y' ORDER BY "levels" NULLS FIRST, "m1"."sort" ASC NULLS FIRST
SELECT "notice_id", "title", "content", "create_time", "create_user", "update_time", "update_user" FROM "sys_notice"
SELECT "dept_id" AS "deptid", "pid" AS "pid", "pids" AS "pids", "simple_name" AS "simplename", "full_name" AS "fullname", "description" AS "description", "version" AS "version", "sort" AS "sort", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_dept" WHERE 1 = 1 AND "pids" LIKE CONCAT('%$[', 1188370943232311297, '$]%') ESCAPE '$'

SELECT "role_id" AS "id", "pid" AS "pid", "name" AS "name", (CASE WHEN ("pid" = 0 OR "pid" IS NULL) THEN 'true' ELSE 'false' END) AS "open" FROM "sys_role"
SELECT COUNT(1) FROM "sys_dict_type"
SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_dict_type" WHERE ("code" = 'd' OR "name" = 'd')
SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE ("code" = 'd' OR "name" = 'd') ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_dict_type" WHERE ("system_flag" = 'Y')
SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE ("system_flag" = 'Y') ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT "dict_type_id", "code", "name", "description", "system_flag", "status", "sort", "create_time", "create_user", "update_time", "update_user" FROM "sys_dict_type" WHERE ("system_flag" = 'N')
SELECT "menu_id" AS "menuid", "code" AS "code", "pcode" AS "pcode", "pcodes" AS "pcodes", "name" AS "name", "icon" AS "icon", "url" AS "url", "sort" AS "sort", "levels" AS "levels", "menu_flag" AS "menuflag", "description" AS "description", "status" AS "status", "new_page_flag" AS "newpageflag", "open_flag" AS "openflag", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_menu" WHERE "status" = 'ENABLE'
SELECT "menu_id" AS "menuid", "code" AS "code", "pcode" AS "pcode", "pcodes" AS "pcodes", "name" AS "name", "icon" AS "icon", "url" AS "url", "sort" AS "sort", "levels" AS "levels", "menu_flag" AS "menuflag", "description" AS "description", "status" AS "status", "new_page_flag" AS "newpageflag", "open_flag" AS "openflag", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_menu" WHERE "status" = 'ENABLE' AND "levels" = '2'
SELECT "operation_log_id", "log_type", "log_name", "user_id", "class_name", "method", "create_time", "succeed", "message" FROM "sys_operation_log" WHERE "operation_log_id" = 1188369905288871937




SELECT COUNT(1) FROM "sys_role"
SELECT "role_id" AS "roleid", "pid" AS "pid", "name" AS "name", "description" AS "description", "sort" AS "sort", "version" AS "version", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_role" ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0
SELECT COUNT(1) FROM "sys_role" WHERE "name" LIKE CONCAT('%', '', '%')
SELECT "role_id" AS "roleid", "pid" AS "pid", "name" AS "name", "description" AS "description", "sort" AS "sort", "version" AS "version", "create_time" AS "createtime", "update_time" AS "updatetime", "create_user" AS "createuser", "update_user" AS "updateuser" FROM "sys_role" WHERE "name" LIKE CONCAT('%', '', '%') ORDER BY "sort" ASC NULLS FIRST LIMIT 10 OFFSET 0



SELECT "file_id", "file_data", "create_time", "update_time", "create_user", "update_user" FROM "sys_file_info" WHERE "file_id" = '1'
SELECT "m1"."menu_id" AS "id", "m1"."icon" AS "icon", (CASE WHEN ("m2"."menu_id" = 0 OR "m2"."menu_id" IS NULL) THEN 0 ELSE "m2"."menu_id" END) AS "parentid", "m1"."name" AS "name", "m1"."url" AS "url", "m1"."levels" AS "levels", "m1"."menu_flag" AS "ismenu", "m1"."sort" AS "num" FROM "sys_menu" AS "m1" INNER JOIN "sys_relation" AS "rela" ON "rela"."menu_id" = "m1"."menu_id" LEFT JOIN "sys_menu" AS "m2" ON "m1"."pcode" = "m2"."code" WHERE "m1"."menu_flag" = 'Y' AND "rela"."role_id" IN (1) ORDER BY "levels" NULLS FIRST, "m1"."sort" ASC NULLS FIRST
