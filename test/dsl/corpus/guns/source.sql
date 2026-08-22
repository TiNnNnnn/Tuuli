select URL from        sys_relation rel        inner join sys_menu m on rel.menu_id = m.menu_id        where rel.role_id = 1
SELECT role_id,pid,name,description,sort,version,create_time,update_time,create_user,update_user FROM sys_role WHERE role_id=1
SELECT dept_id,pid,pids,simple_name,full_name,description,version,sort,create_time,update_time,create_user,update_user FROM sys_dept WHERE dept_id=25

SELECT user_id,avatar,account,password,salt,name,birthday,sex,email,phone,role_id,dept_id,status,create_time,create_user,update_time,update_user,version FROM sys_user WHERE user_id=1188369796975165441

SELECT COUNT(1) FROM sys_notice
select                 notice_id AS "noticeId", title AS "title", content AS "content", create_time AS "createTime", create_user AS "createUser", update_time AS "updateTime", update_user AS "updateUser"             from sys_notice                 order by create_time DESC LIMIT 0,10
SELECT COUNT(1) FROM sys_notice WHERE title LIKE CONCAT('%', 'd', '%') OR content LIKE CONCAT('%', 'd', '%')
select                 notice_id AS "noticeId", title AS "title", content AS "content", create_time AS "createTime", create_user AS "createUser", update_time AS "updateTime", update_user AS "updateUser"             from sys_notice                     where title like CONCAT('%','d','%') or content like CONCAT('%','d','%')                 order by create_time DESC LIMIT 0,10
select                 notice_id AS "noticeId", title AS "title", content AS "content", create_time AS "createTime", create_user AS "createUser", update_time AS "updateTime", update_user AS "updateUser"             from sys_notice                     where title like CONCAT('%','d','%') or content like CONCAT('%','d','%')                 order by create_time DESC

SELECT dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user FROM sys_dict_type WHERE dict_type_id=1188371218487705601
SELECT COUNT( 1 ) FROM sys_dict_type   WHERE ( code = 'NOUSE' OR name = 'd' ) AND ( dict_type_id <> 1188371218487705601 )

select                          user_id AS "userId", avatar AS "avatar", account AS "account", salt AS "salt", password AS "password", name AS "name", birthday AS "birthday", sex AS "sex", email AS "email", phone AS "phone", role_id AS "roleId", dept_id AS "deptId", status AS "status", create_time AS "createTime", create_user AS "createUser", update_time AS "updateTime", update_user AS "updateUser", version AS "version"                 from sys_user where account = 'test1' and status != 'DELETED'

SELECT notice_id,title,content,create_time,create_user,update_time,update_user FROM sys_notice WHERE notice_id=1188376366811316226



SELECT menu_id,code,pcode,pcodes,name,icon,url,sort,levels,menu_flag,description,status,new_page_flag,open_flag,create_time,update_time,create_user,update_user FROM sys_menu WHERE menu_id=1188371829954314242


select                 menu_id AS "menuId", code AS "code", pcode AS "pcode", pcodes AS "pcodes", name AS "name", icon AS "icon", url AS "url", sort AS "sort", levels AS "levels", menu_flag AS "menuFlag", description AS "description", status AS "status", new_page_flag AS "newPageFlag", open_flag AS "openFlag", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_menu where 1 = 1                     and pcodes LIKE CONCAT('%$[','142','$]%') escape '$'
SELECT        r.role_id as id,        r.pid,        r.name,        (        CASE        WHEN (r.pid = 0 OR r.pid IS NULL) THEN        'true'        ELSE        'false'        END        ) as "open",        (        CASE        WHEN (r1.role_id = 0 OR r1.role_id IS NULL) THEN        'false'        ELSE        'true'        END        ) as "checked"        FROM        sys_role r        LEFT JOIN (        SELECT        role_id        FROM        sys_role        WHERE        role_id IN         (              1         )         ) r1 ON r.role_id = r1.role_id        ORDER BY pid,sort ASC
SELECT COUNT(1) FROM sys_operation_log WHERE 1 = 1
select                 operation_log_id AS "operationLogId", log_type AS "logType", log_name AS "logName", user_id AS "userId", class_name AS "className", method AS "method", create_time AS "createTime", succeed AS "succeed", message AS "message"             from sys_operation_log where 1 = 1 LIMIT 0,10
SELECT COUNT(1) FROM sys_operation_log WHERE 1 = 1 AND log_type LIKE CONCAT('%', 'd', '%')
select                 operation_log_id AS "operationLogId", log_type AS "logType", log_name AS "logName", user_id AS "userId", class_name AS "className", method AS "method", create_time AS "createTime", succeed AS "succeed", message AS "message"             from sys_operation_log where 1 = 1                                       and log_type like CONCAT('%','d','%') LIMIT 0,10
SELECT COUNT(1) FROM sys_operation_log WHERE 1 = 1 AND (create_time BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59')) AND log_type LIKE CONCAT('%', 'd', '%')
select                 operation_log_id AS "operationLogId", log_type AS "logType", log_name AS "logName", user_id AS "userId", class_name AS "className", method AS "method", create_time AS "createTime", succeed AS "succeed", message AS "message"             from sys_operation_log where 1 = 1                     and (create_time between CONCAT('2019-07-08',' 00:00:00') and CONCAT('2019-10-31',' 23:59:59'))                                       and log_type like CONCAT('%','d','%') LIMIT 0,10
SELECT COUNT(1) FROM sys_user WHERE status != 'DELETED'
select                 user_id AS "userId", avatar AS "avatar", account AS "account", salt AS "salt", name AS "name", birthday AS "birthday", sex AS "sex", email AS "email", phone AS "phone", role_id AS "roleId", dept_id AS "deptId", status AS "status", create_time AS "createTime", create_user AS "createUser", update_time AS "updateTime", update_user AS "updateUser", version AS "version"             from sys_user        where status != 'DELETED' LIMIT 0,10
SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type   WHERE  name='d'
SELECT  dict_id,dict_type_id,code,name,parent_id,parent_ids,status,sort,description,create_time,update_time,create_user,update_user  FROM sys_dict   WHERE dict_type_id = 1106120208097067009
SELECT  menu_id,code,pcode,pcodes,name,icon,url,sort,levels,menu_flag,description,status,new_page_flag,open_flag,create_time,update_time,create_user,update_user  FROM sys_menu   WHERE  code='1'

SELECT COUNT(1) FROM sys_dept WHERE 1 = 1
select                 dept_id AS "deptId", pid AS "pid", pids AS "pids", simple_name AS "simpleName", full_name AS "fullName", description AS "description", version AS "version", sort AS "sort", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_dept where 1 = 1                          order by sort ASC LIMIT 0,10
SELECT COUNT(1) FROM sys_dept WHERE 1 = 1 AND (dept_id = 24 OR dept_id IN (SELECT dept_id FROM sys_dept WHERE pids LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$'))
select                 dept_id AS "deptId", pid AS "pid", pids AS "pids", simple_name AS "simpleName", full_name AS "fullName", description AS "description", version AS "version", sort AS "sort", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_dept where 1 = 1                              and (dept_id = 24 or dept_id in ( select dept_id from sys_dept where pids like CONCAT('%$[', 24, '$]%') escape '$' ))                 order by sort ASC LIMIT 0,10
SELECT COUNT(1) FROM sys_dept WHERE 1 = 1 AND simple_name LIKE CONCAT('%', 'd', '%') OR full_name LIKE CONCAT('%', 'd', '%') AND (dept_id = 24 OR dept_id IN (SELECT dept_id FROM sys_dept WHERE pids LIKE CONCAT('%$[', 24, '$]%') ESCAPE '$'))
select                 dept_id AS "deptId", pid AS "pid", pids AS "pids", simple_name AS "simpleName", full_name AS "fullName", description AS "description", version AS "version", sort AS "sort", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_dept where 1 = 1                     and simple_name like CONCAT('%','d','%') or full_name like CONCAT('%','d','%')                              and (dept_id = 24 or dept_id in ( select dept_id from sys_dept where pids like CONCAT('%$[', 24, '$]%') escape '$' ))                 order by sort ASC LIMIT 0,10
select menu_id from        sys_relation where role_id = 1188370535139115009
SELECT        m1.menu_id AS id,        (        CASE        WHEN (m2.menu_id = 0 OR m2.menu_id IS NULL) THEN        0        ELSE        m2.menu_id        END        ) AS pId,        m1.name        AS name,        (        CASE        WHEN (m2.menu_id = 0 OR m2.menu_id IS NULL) THEN        'true'        ELSE        'false'        END        ) as "open"        FROM        sys_menu m1        LEFT join sys_menu m2 ON m1.pcode = m2.code        ORDER BY        m1.menu_id ASC
select dept_id AS id, pid as "pId", simple_name as name,		(		CASE		WHEN (pid = 0 OR pid IS NULL) THEN		'true'		ELSE		'false'		END		) as "open" from sys_dept
SELECT COUNT(1) FROM sys_login_log WHERE 1 = 1
select                 login_log_id AS "loginLogId", log_name AS "logName", user_id AS "userId", create_time AS "createTime", succeed AS "succeed", message AS "message", ip_address AS "ipAddress"             from sys_login_log        where 1 = 1 LIMIT 0,10
SELECT COUNT(1) FROM sys_login_log WHERE 1 = 1 AND (create_time BETWEEN CONCAT('2019-07-08', ' 00:00:00') AND CONCAT('2019-10-31', ' 23:59:59'))
select                 login_log_id AS "loginLogId", log_name AS "logName", user_id AS "userId", create_time AS "createTime", succeed AS "succeed", message AS "message", ip_address AS "ipAddress"             from sys_login_log        where 1 = 1                     and (create_time between CONCAT('2019-07-08',' 00:00:00') and CONCAT('2019-10-31',' 23:59:59')) LIMIT 0,10





SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type   WHERE code = 'NOUSE' OR name = 'e'


SELECT        m1.menu_id AS id,        m1.icon AS icon,        (        CASE        WHEN (m2.menu_id = 0 OR m2.menu_id IS NULL) THEN        0        ELSE        m2.menu_id        END        ) AS "parentId",        m1.name as name,        m1.url as url,        m1.levels as levels,        m1.menu_flag as ismenu,        m1.sort as num        FROM        sys_menu m1        LEFT join sys_menu m2 ON m1.pcode = m2.code        INNER JOIN (        SELECT        menu_id        FROM        sys_menu        WHERE        menu_id IN (        SELECT        menu_id        FROM        sys_relation rela        WHERE        rela.role_id IN         (              1         )         )        ) m3 ON m1.menu_id = m3.menu_id        where m1.menu_flag = 'Y'        order by levels,m1.sort asc
SELECT  notice_id,title,content,create_time,create_user,update_time,update_user  FROM sys_notice
select                 dept_id AS "deptId", pid AS "pid", pids AS "pids", simple_name AS "simpleName", full_name AS "fullName", description AS "description", version AS "version", sort AS "sort", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_dept where 1 = 1                     and pids LIKE CONCAT('%$[',1188370943232311297,'$]%') escape '$'

select role_id AS id, pid as "pId",		name as name, (case when (pid = 0 or pid is null) then 'true'		else 'false' end) as "open" from sys_role
SELECT COUNT(1) FROM sys_dict_type
SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type ORDER BY sort ASC LIMIT 0,10
SELECT COUNT(1) FROM sys_dict_type WHERE (code = 'd' OR name = 'd')
SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type   WHERE ( code = 'd' OR name = 'd' ) ORDER BY sort ASC LIMIT 0,10
SELECT COUNT(1) FROM sys_dict_type WHERE (system_flag = 'Y')
SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type   WHERE ( system_flag = 'Y' ) ORDER BY sort ASC LIMIT 0,10
SELECT  dict_type_id,code,name,description,system_flag,status,sort,create_time,create_user,update_time,update_user  FROM sys_dict_type   WHERE ( system_flag = 'N' )
select                 menu_id AS "menuId", code AS "code", pcode AS "pcode", pcodes AS "pcodes", name AS "name", icon AS "icon", url AS "url", sort AS "sort", levels AS "levels", menu_flag AS "menuFlag", description AS "description", status AS "status", new_page_flag AS "newPageFlag", open_flag AS "openFlag", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_menu        where status = 'ENABLE'
select                 menu_id AS "menuId", code AS "code", pcode AS "pcode", pcodes AS "pcodes", name AS "name", icon AS "icon", url AS "url", sort AS "sort", levels AS "levels", menu_flag AS "menuFlag", description AS "description", status AS "status", new_page_flag AS "newPageFlag", open_flag AS "openFlag", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_menu        where status = 'ENABLE'                              and levels = '2'
SELECT operation_log_id,log_type,log_name,user_id,class_name,method,create_time,succeed,message FROM sys_operation_log WHERE operation_log_id=1188369905288871937




SELECT COUNT(1) FROM sys_role
select                 role_id AS "roleId", pid AS "pid", name AS "name", description AS "description", sort AS "sort", version AS "version", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_role                 order by sort asc LIMIT 0,10
SELECT COUNT(1) FROM sys_role WHERE name LIKE CONCAT('%', '', '%')
select                 role_id AS "roleId", pid AS "pid", name AS "name", description AS "description", sort AS "sort", version AS "version", create_time AS "createTime", update_time AS "updateTime", create_user AS "createUser", update_user AS "updateUser"             from sys_role                     where name like CONCAT('%','','%')                 order by sort asc LIMIT 0,10



SELECT file_id,file_data,create_time,update_time,create_user,update_user FROM sys_file_info WHERE file_id='1'
SELECT m1.menu_id AS id, m1.icon AS icon, ( CASE WHEN ( m2.menu_id = 0 OR m2.menu_id IS NULL ) THEN 0 ELSE m2.menu_id END ) AS "parentId", m1.name as name, m1.url as url, m1.levels as levels, m1.menu_flag as ismenu, m1.sort as num FROM sys_menu m1 INNER JOIN sys_relation rela ON rela.menu_id = m1.menu_id LEFT join sys_menu m2 ON m1.pcode = m2.code where m1.menu_flag = 'Y' and rela.role_id IN (1) order by levels, m1.sort asc
