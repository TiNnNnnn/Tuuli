
SELECT COUNT(1) FROM t_job_log
SELECT LOG_ID, job_id, bean_name, method_name, params, status, error, times, create_time FROM t_job_log ORDER BY create_time DESC LIMIT 0,10


SELECT * FROM QRTZ_LOCKS WHERE SCHED_NAME = 'FEBS_Scheduler' AND LOCK_NAME = 'TRIGGER_ACCESS' FOR UPDATE
SELECT JOB_NAME FROM QRTZ_JOB_DETAILS WHERE SCHED_NAME = 'FEBS_Scheduler' AND JOB_NAME = 'TASK_12' AND JOB_GROUP = 'DEFAULT'

SELECT TRIGGER_NAME FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'TASK_12' AND TRIGGER_GROUP = 'DEFAULT'
SELECT TRIGGER_GROUP FROM QRTZ_PAUSED_TRIGGER_GRPS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_GROUP = 'DEFAULT'


SELECT TRIGGER_NAME, TRIGGER_GROUP FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND JOB_NAME = 'TASK_12' AND JOB_GROUP = 'DEFAULT'
SELECT * FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'TASK_12' AND TRIGGER_GROUP = 'DEFAULT'
SELECT * FROM QRTZ_CRON_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'TASK_12' AND TRIGGER_GROUP = 'DEFAULT'
SELECT TRIGGER_STATE FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'TASK_12' AND TRIGGER_GROUP = 'DEFAULT'


SELECT DEPT_ID, PARENT_ID, DEPT_NAME, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_dept WHERE PARENT_ID IN ('11')



SELECT ROLE_ID, ROLE_NAME, REMARK, CREATE_TIME, MODIFY_TIME FROM t_role
SELECT COUNT(1) FROM t_log
SELECT ID, USERNAME, OPERATION, TIME, METHOD, PARAMS, IP, CREATE_TIME, LOCATION FROM t_log ORDER BY create_time DESC LIMIT 0,10
SELECT COUNT(1) FROM t_log WHERE CREATE_TIME >= '2019-10-01' AND CREATE_TIME <= '2019-11-21'
SELECT ID, USERNAME, OPERATION, TIME, METHOD, PARAMS, IP, CREATE_TIME, LOCATION FROM t_log WHERE CREATE_TIME >= '2019-10-01' AND CREATE_TIME <= '2019-11-21' ORDER BY create_time DESC LIMIT 0,10

SELECT DEPT_ID, PARENT_ID, DEPT_NAME, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_dept ORDER BY ORDER_NUM ASC

SELECT DEPT_ID, PARENT_ID, DEPT_NAME, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_dept WHERE DEPT_NAME = 'e' ORDER BY ORDER_NUM ASC
SELECT COUNT(1) FROM t_login_log
SELECT ID, USERNAME, LOGIN_TIME, LOCATION, IP, `SYSTEM`, BROWSER FROM t_login_log ORDER BY login_time DESC LIMIT 0,10
SELECT COUNT(1) FROM t_login_log WHERE LOGIN_TIME >= '2019-10-09' AND LOGIN_TIME <= '2019-11-15'
SELECT ID, USERNAME, LOGIN_TIME, LOCATION, IP, `SYSTEM`, BROWSER FROM t_login_log WHERE LOGIN_TIME >= '2019-10-09' AND LOGIN_TIME <= '2019-11-15' ORDER BY login_time DESC LIMIT 0,10
SELECT u.user_id userId, u.username, u.email, u.mobile, u.password, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, u.theme, u.is_tab isTab, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE u.username = 'mrbird' GROUP BY u.username, u.user_id, u.email, u.mobile, u.password, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar, u.theme, u.is_tab

SELECT COUNT(1) FROM t_eximport
SELECT field1, field2, field3, create_time FROM t_eximport LIMIT 10,10
SELECT JOB_ID, bean_name, method_name, params, cron_expression, status, remark, create_time FROM t_job WHERE JOB_ID = 12
SELECT r.* FROM t_role r LEFT JOIN t_user_role ur ON (r.role_id = ur.role_id) LEFT JOIN t_user u ON (u.user_id = ur.user_id) WHERE u.username = 'MrBird'
SELECT m.perms FROM t_role r LEFT JOIN t_user_role ur ON (r.role_id = ur.role_id) LEFT JOIN t_user u ON (u.user_id = ur.user_id) LEFT JOIN t_role_menu rm ON (rm.role_id = r.role_id) LEFT JOIN t_menu m ON (m.menu_id = rm.menu_id) WHERE u.username = 'MrBird' AND m.perms IS NOT NULL AND m.perms <> ''
SELECT ID, author, base_package, entity_package, mapper_package, mapper_xml_package, service_package, service_impl_package, controller_package, is_trim, trim_value FROM t_generator_config WHERE ID = '1'

SELECT COUNT(1) FROM t_job
SELECT JOB_ID, bean_name, method_name, params, cron_expression, status, remark, create_time FROM t_job ORDER BY create_time DESC LIMIT 0,10





SELECT COUNT(1) FROM ( SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ) TOTAL
SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ORDER BY userId ASC LIMIT 0,5
SELECT MENU_ID, PARENT_ID, MENU_NAME, URL, PERMS, ICON, TYPE, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_menu ORDER BY MENU_ID ASC, ORDER_NUM ASC


SELECT MENU_ID, PARENT_ID, MENU_NAME, URL, PERMS, ICON, TYPE, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_menu WHERE PARENT_ID IN ('178')



SELECT * FROM QRTZ_SIMPLE_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'MT_6qmdud2aod6t6' AND TRIGGER_GROUP = 'DEFAULT'
SELECT J.JOB_NAME, J.JOB_GROUP, J.IS_DURABLE, J.JOB_CLASS_NAME, J.REQUESTS_RECOVERY FROM QRTZ_TRIGGERS T, QRTZ_JOB_DETAILS J WHERE T.SCHED_NAME = 'FEBS_Scheduler' AND J.SCHED_NAME = 'FEBS_Scheduler' AND T.TRIGGER_NAME = 'MT_6qmdud2aod6t6' AND T.TRIGGER_GROUP = 'DEFAULT' AND T.JOB_NAME = J.JOB_NAME AND T.JOB_GROUP = J.JOB_GROUP


SELECT COUNT(TRIGGER_NAME) FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND JOB_NAME = 'TASK_12' AND JOB_GROUP = 'DEFAULT'



SELECT DEPT_ID, PARENT_ID, DEPT_NAME, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_dept ORDER BY order_num ASC


SELECT MENU_ID, PARENT_ID, MENU_NAME, URL, PERMS, ICON, TYPE, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_menu ORDER BY ORDER_NUM ASC



SELECT * FROM QRTZ_JOB_DETAILS WHERE SCHED_NAME = 'FEBS_Scheduler' AND JOB_NAME = 'TASK_12' AND JOB_GROUP = 'DEFAULT'







SELECT count(1) FROM t_login_log WHERE datediff(login_time, now()) = 0
SELECT count(DISTINCT (ip)) FROM t_login_log WHERE datediff(login_time, now()) = 0
SELECT date_format(l.login_time, '%m-%d') days, count(1) count FROM (SELECT * FROM t_login_log WHERE date_sub(curdate(), INTERVAL 10 day) <= date(login_time)) AS l WHERE 1 = 1 GROUP BY days
SELECT date_format(l.login_time, '%m-%d') days, count(1) count FROM (SELECT * FROM t_login_log WHERE date_sub(curdate(), INTERVAL 10 day) <= date(login_time)) AS l WHERE 1 = 1 AND l.username = 'MrBird' GROUP BY days
SELECT ID, author, base_package, entity_package, mapper_package, mapper_xml_package, service_package, service_impl_package, controller_package, is_trim, trim_value FROM t_generator_config
SELECT TRIGGER_STATE, NEXT_FIRE_TIME, JOB_NAME, JOB_GROUP FROM QRTZ_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND TRIGGER_NAME = 'TASK_12' AND TRIGGER_GROUP = 'DEFAULT'
SELECT * FROM QRTZ_FIRED_TRIGGERS WHERE SCHED_NAME = 'FEBS_Scheduler' AND JOB_NAME = 'TASK_12' AND JOB_GROUP = 'DEFAULT'

SELECT DEPT_ID, PARENT_ID, DEPT_NAME, ORDER_NUM, CREATE_TIME, MODIFY_TIME FROM t_dept
SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ORDER BY userId ASC LIMIT 0,10
SELECT COUNT(1) FROM ( SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.username = 'Jana' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ) TOTAL
SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.username = 'Jana' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ORDER BY userId ASC LIMIT 0,10
SELECT COUNT(1) FROM ( SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.ssex = '1' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ) TOTAL
SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.ssex = '1' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ORDER BY userId ASC LIMIT 0,10
SELECT COUNT(1) FROM ( SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.mobile = '17711111111' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ) TOTAL
SELECT u.user_id userId, u.username, u.email, u.mobile, u.status, u.create_time createTime, u.ssex sex, u.dept_id deptId, u.last_login_time lastLoginTime, u.modify_time modifyTime, u.description, u.avatar, d.dept_name deptName, GROUP_CONCAT(r.role_id) roleId, GROUP_CONCAT(r.ROLE_NAME) roleName FROM t_user u LEFT JOIN t_dept d ON (u.dept_id = d.dept_id) LEFT JOIN t_user_role ur ON (u.user_id = ur.user_id) LEFT JOIN t_role r ON r.role_id = ur.role_id WHERE 1 = 1 AND u.mobile = '17711111111' GROUP BY u.username, u.user_id, u.email, u.mobile, u.status, u.create_time, u.ssex, u.dept_id, u.last_login_time, u.modify_time, u.description, u.avatar ORDER BY userId ASC LIMIT 0,5

SELECT COUNT(1) FROM ( SELECT r.role_id roleId, r.role_name roleName, r.remark, r.create_time createTime, r.modify_time modifyTime, GROUP_CONCAT(rm.menu_id) menuIds FROM t_role r LEFT JOIN t_role_menu rm ON (r.role_id = rm.role_id) WHERE 1 = 1 GROUP BY r.role_id ) TOTAL
SELECT r.role_id roleId, r.role_name roleName, r.remark, r.create_time createTime, r.modify_time modifyTime, GROUP_CONCAT(rm.menu_id) menuIds FROM t_role r LEFT JOIN t_role_menu rm ON (r.role_id = rm.role_id) WHERE 1 = 1 GROUP BY r.role_id ORDER BY createTime DESC LIMIT 0,10
SELECT r.role_id roleId, r.role_name roleName, r.remark, r.create_time createTime, r.modify_time modifyTime, GROUP_CONCAT(rm.menu_id) menuIds FROM t_role r LEFT JOIN t_role_menu rm ON (r.role_id = rm.role_id) WHERE 1 = 1 GROUP BY r.role_id ORDER BY createTime DESC LIMIT 0,5
SELECT m.* FROM t_menu m WHERE m.type <> 1 AND m.MENU_ID IN (SELECT DISTINCT rm.menu_id FROM t_role_menu rm LEFT JOIN t_role r ON (rm.role_id = r.role_id) LEFT JOIN t_user_role ur ON (ur.role_id = r.role_id) LEFT JOIN t_user u ON (u.user_id = ur.user_id) WHERE u.username = 'MrBird') ORDER BY m.order_num
SELECT count(DISTINCT ip) FROM t_login_log WHERE login_time between curdate() and curdate() + interval 1 day - interval 1 second
SELECT count(1) FROM t_login_log WHERE login_time between curdate() and curdate() + interval 1 day - interval 1 second
