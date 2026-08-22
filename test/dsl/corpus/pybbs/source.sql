SELECT  id,`key`,`value`,description,pid,`type`,`option`,`reboot`  FROM system_config
select u.*, r.name as roleName from admin_user u left join role r on u.role_id = r.id
SELECT  id,username,password,in_time,role_id  FROM admin_user   WHERE username = 'admin'
SELECT id,name FROM role WHERE id=1
SELECT  role_id,permission_id  FROM role_permission   WHERE role_id = 1
SELECT  id,name,value,pid  FROM permission   WHERE id IN (11,12,13,14,15,16,45,46,48,49,17,18,19,20,21,22,23,24,43,25,26,27,28,29,30,31,32,44,38,39,40,41,35,36,37,42,51,52,53,54,55)
SELECT  id,`key`,`value`,description,pid,`type`,`option`,`reboot`  FROM system_config   WHERE `key` = 'redis_host'
SELECT id,title,content,in_time,modify_time,user_id,comment_count,collect_count,view,top,good,up_ids FROM topic WHERE id=2
SELECT  id,username,telegram_name,avatar,password,email,mobile,website,bio,score,in_time,token,email_notification,active  FROM user   WHERE token = '0234c153-fc73-4901-a4e5-edc7a55a603d'
SELECT  id,title,content,in_time,modify_time,user_id,comment_count,collect_count,view,top,good,up_ids  FROM topic   WHERE title = 'd'


SELECT id,topic_id,user_id,content,in_time,comment_id,up_ids FROM comment WHERE id=1


select count(1)    from topic t    where to_days(t.in_time) = to_days(now())
select count(1) from tag where to_days(in_time) = to_days(now())
select count(1) from comment where to_days(in_time) = to_days(now())
select count(1) from user where to_days(in_time) = to_days(now())
SELECT  id,username,telegram_name,avatar,password,email,mobile,website,bio,score,in_time,token,email_notification,active  FROM user   WHERE username = 'stephen3'
SELECT  id,oauth_id,`type`,login,access_token,in_time,bio,email,user_id,refresh_token,union_id,expires_in  FROM oauth_user   WHERE user_id = 3
SELECT COUNT( 1 ) FROM collect   WHERE user_id = 3

SELECT COUNT(1) FROM user
SELECT  id,username,telegram_name,avatar,password,email,mobile,website,bio,score,in_time,token,email_notification,active  FROM user ORDER BY in_time DESC LIMIT 0,20
SELECT COUNT(1) FROM user WHERE username = 'd'
SELECT  id,name  FROM role

SELECT id,username,telegram_name,avatar,password,email,mobile,website,bio,score,in_time,token,email_notification,active FROM user WHERE id=1

SELECT COUNT(1) FROM topic t LEFT JOIN user u ON t.user_id = u.id
select t.*, u.username    from topic t    left join user u on t.user_id = u.id          order by t.in_time desc LIMIT 0,20
SELECT COUNT(1) FROM topic t LEFT JOIN user u ON t.user_id = u.id WHERE t.in_time BETWEEN '2019-10-11' AND '2019-10-26'
select t.*, u.username    from topic t    left join user u on t.user_id = u.id     WHERE t.in_time between '2019-10-11' and '2019-10-26'     order by t.in_time desc LIMIT 0,20
SELECT  tag_id,topic_id  FROM topic_tag   WHERE topic_id = 3
SELECT  topic_id,user_id,in_time  FROM collect   WHERE topic_id = 3
SELECT  topic_id,user_id,in_time  FROM collect   WHERE topic_id = 3 AND user_id = 1





SELECT COUNT(1) FROM tag
SELECT  id,name,description,icon,topic_count,in_time  FROM tag       ORDER BY topic_count DESC

SELECT COUNT(1) FROM sensitive_word
SELECT  id,word  FROM sensitive_word
SELECT COUNT(1) FROM sensitive_word WHERE word = 'fuck'
SELECT  id,word  FROM sensitive_word   WHERE word = 'fuck'
SELECT  id,name,value,pid  FROM permission   WHERE pid = 0
SELECT COUNT(1) FROM comment c LEFT JOIN topic t ON c.topic_id = t.id LEFT JOIN user u ON u.id = c.user_id
select c.*, t.title, t.id as topicId, u.username    from comment c    left join topic t on c.topic_id = t.id    left join user u on u.id = c.user_id          order by c.in_time desc LIMIT 0,20
SELECT COUNT(1) FROM comment c LEFT JOIN topic t ON c.topic_id = t.id LEFT JOIN user u ON u.id = c.user_id WHERE c.in_time BETWEEN '2019-10-01' AND '2019-10-31'
select c.*, t.title, t.id as topicId, u.username    from comment c    left join topic t on c.topic_id = t.id    left join user u on u.id = c.user_id     WHERE c.in_time between '2019-10-01' and '2019-10-31'     order by c.in_time desc LIMIT 0,20
SELECT  id,username,telegram_name,avatar,password,email,mobile,website,bio,score,in_time,token,email_notification,active  FROM user   WHERE email = '321@qq.com'

SELECT  id,user_id,code,in_time,expire_time,email,mobile,used  FROM code   WHERE email = '321@qq.com' AND user_id = 3 AND used = 0 AND expire_time > '2019-10-21 19:50:16.271'
SELECT  id,user_id,code,in_time,expire_time,email,mobile,used  FROM code   WHERE code = '378640'






select count(1) from notification where target_user_id = 3 and `read` = false
select count(1) from user where in_time between curdate() and curdate() + interval 1 day - interval 1 second
