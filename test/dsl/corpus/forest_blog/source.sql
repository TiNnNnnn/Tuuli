SELECT         menu_id, menu_name, menu_url, menu_level, menu_icon, menu_order       FROM  menu     ORDER BY menu_order DESC, menu_id ASC
SELECT             category_id, category_pid, category_name, category_description, category_order, category_icon           FROM         category         order by category_order desc,category_id asc
SELECT COUNT(*) FROM         article         WHERE article_status = 1
SELECT SUM(article_comment_count)        FROM         article         WHERE article_status = 1
SELECT COUNT(*) FROM         category
SELECT COUNT(*) FROM  tag
SELECT COUNT(*) FROM  link      WHERE link_status=1
SELECT SUM(article_view_count) FROM         article         WHERE article_status = 1
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article         WHERE        article_status = 1 AND article_update_time=        (        SELECT max(article_update_time) FROM article        )
SELECT * FROM         options         limit 1
select             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           from         comment         where comment_id = 31




select          notice_id, notice_title, notice_content, notice_create_time, notice_update_time,     notice_status, notice_order       from  notice     where notice_id = 3


select             user_id, user_name, user_pass, user_nickname, user_email, user_url, user_avatar,     user_last_login_ip, user_register_time, user_last_login_time, user_status           from  user         where user_id = 1


SELECT         link_id, link_url, link_name, link_image, link_description, link_owner_nickname,     link_owner_contact, link_update_time, link_create_time, link_order, link_status       FROM   link           ORDER BY link_status ASC,link_order DESC,link_id ASC

select count(*) from         article_category_ref         where category_id = 100000007


SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article         WHERE article_status = 1        ORDER BY        article_comment_count DESC,article_order DESC, article_id DESC        limit 8
SELECT             user_id, user_name, user_pass, user_nickname, user_email, user_url, user_avatar,     user_last_login_ip, user_register_time, user_last_login_time, user_status           FROM         user          WHERE user_name='admin' OR user_email='admin'         AND user_status>0        limit 1

select          menu_id, menu_name, menu_url, menu_level, menu_icon, menu_order       from  menu     where menu_id = 3

SELECT         page_id, page_key, page_title, page_content, page_create_time, page_update_time,     page_view_count, page_comment_count, page_status       FROM     page      WHERE page_status=1 AND            page_key='aboutSite'
SELECT        article_id, article_user_id, article_title, article_view_count, article_comment_count, article_like_count,        article_create_time,        article_update_time, article_is_comment, article_status, article_order        FROM         article         ORDER BY article_id DESC        LIMIT 5
SELECT             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           FROM         comment         WHERE comment_role = 0        ORDER BY comment_id DESC        LIMIT 5
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article          WHERE article_status = 1 AND                        article_id = 6
SELECT             user_id, user_name, user_pass, user_nickname, user_email, user_url, user_avatar,     user_last_login_ip, user_register_time, user_last_login_time, user_status           FROM         user         ORDER BY `user_status` ASC
SELECT COUNT(*)        FROM         article         WHERE article_user_id=1 AND article_status = 1





SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article          WHERE article_id = 7
SELECT        category.category_id, category.category_pid, category.category_name        FROM category, article_category_ref        WHERE article_category_ref.article_id = 7 AND        article_category_ref.category_id = category.category_id        ORDER BY category.category_pid asc
SELECT tag.* FROM tag, article_tag_ref    WHERE article_tag_ref.article_id = 7 AND    article_tag_ref.tag_id = tag.tag_id
SELECT         tag_id, tag_name, tag_description       FROM  tag
SELECT         page_id, page_key, page_title, page_content, page_create_time, page_update_time,     page_view_count, page_comment_count, page_status       FROM     page      WHERE page_key='aboutSite'

select count(*) from  article_tag_ref     where tag_id = 17



SELECT             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           FROM         comment         WHERE        comment_pid=33


select          page_id, page_key, page_title, page_content, page_create_time, page_update_time,     page_view_count, page_comment_count, page_status       from  page     where page_id = 5



SELECT        article_id, article_user_id, article_title, article_create_time        FROM         article         WHERE article_status = 1        ORDER BY article_id DESC
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article         WHERE article_status = 1        ORDER BY        article_comment_count DESC,article_order DESC, article_id DESC        limit 10
SELECT             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           FROM         comment         WHERE        comment_article_id = 33        ORDER BY comment_id ASC
SELECT category_id FROM         article_category_ref         WHERE article_id = 33
SELECT        article.article_id, article.article_user_id, article.article_title,        article.article_view_count, article.article_comment_count,        article.article_like_count, article.article_create_time, article.article_update_time,        article.article_is_comment, article.article_status, article.article_order,        article.article_summary        FROM article, article_category_ref         WHERE article.article_status = 1 AND            article.article_id = article_category_ref.article_id AND            article_category_ref.category_id                            IN                (                    10                ,                    13                )         LIMIT 5
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article         WHERE article_status = 1        ORDER BY article_view_count DESC,article_order DESC, article_id DESC        limit 5
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article          WHERE article_id > 33 AND article_status = 1         ORDER BY article_id        limit 1
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article          WHERE article_id < 33 AND article_status = 1         ORDER BY article_id        limit 1
SELECT             article_id, article_user_id, article_title, article_content,article_summary, article_view_count, article_comment_count, article_like_count, article_create_time,    article_update_time, article_is_comment, article_status, article_order             FROM         article         WHERE article_status = 1        ORDER BY        RAND()        limit 8
SELECT             user_id, user_name, user_pass, user_nickname, user_email, user_url, user_avatar,     user_last_login_ip, user_register_time, user_last_login_time, user_status           FROM         user          WHERE user_name='zhouz'         limit 1
SELECT count(0) FROM article WHERE article.article_status = 1 AND article.article_title LIKE concat(concat('%', 'MySQL'), '%') AND 1 = 1
SELECT        article.*        FROM        article         WHERE article.article_status = 1 AND                                        article.article_title LIKE concat(concat('%','MySQL'),'%') AND                                                            1 = 1         ORDER BY `article`.`article_order` DESC, `article`.`article_id` DESC limit 0,10
SELECT             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           FROM         comment         WHERE comment_role = 0        ORDER BY comment_id DESC        LIMIT 10


select          tag_id, tag_name, tag_description       from  tag     where tag_id = 38

SELECT count(0) FROM article WHERE article.article_status = 1 AND 1 = 1
SELECT        article.*        FROM        article         WHERE article.article_status = 1 AND                                                                        1 = 1         ORDER BY `article`.`article_order` DESC, `article`.`article_id` DESC limit 0,10
SELECT         notice_id, notice_title, notice_content, notice_create_time, notice_update_time,     notice_status, notice_order       FROM   notice      WHERE notice_status=1     ORDER BY notice_status ASC, notice_order DESC, notice_id ASC
SELECT         link_id, link_url, link_name, link_image, link_description, link_owner_nickname,     link_owner_contact, link_update_time, link_create_time, link_order, link_status       FROM   link      WHERE link_status=1     ORDER BY link_status ASC,link_order DESC,link_id ASC

SELECT count(0) FROM comment
SELECT             comment_id, comment_pid, comment_pname, comment_article_id, comment_author_name,    comment_author_email, comment_author_url, comment_author_avatar, comment_content, comment_agent,     comment_ip,comment_create_time, comment_role           FROM         comment         ORDER BY comment_id DESC limit 0,10
SELECT             user_id, user_name, user_pass, user_nickname, user_email, user_url, user_avatar,     user_last_login_ip, user_register_time, user_last_login_time, user_status           FROM         user          WHERE user_email='zhouz@zhouz.com'         limit 1

select          link_id, link_url, link_name, link_image, link_description, link_owner_nickname,     link_owner_contact, link_update_time, link_create_time, link_order, link_status       from link    where link_id = 6


SELECT         notice_id, notice_title, notice_content, notice_create_time, notice_update_time,     notice_status, notice_order       FROM   notice           ORDER BY notice_status ASC, notice_order DESC, notice_id ASC



SELECT count(0) FROM article WHERE article.article_status = 1 AND article.article_id IN (SELECT article_tag_ref.article_id FROM article_tag_ref WHERE article_tag_ref.tag_id = 1) AND 1 = 1
SELECT        article.*        FROM        article         WHERE article.article_status = 1 AND                                                                            article.article_id IN (                SELECT article_tag_ref.article_id FROM article_tag_ref                WHERE article_tag_ref.tag_id = 1                ) AND                        1 = 1         ORDER BY `article`.`article_order` DESC, `article`.`article_id` DESC limit 0,10

SELECT count(0) FROM article WHERE 1 = 1
SELECT        article.*        FROM        article         WHERE 1 = 1         ORDER BY `article`.`article_order` DESC, `article`.`article_id` DESC limit 0,10
SELECT             category_id, category_pid, category_name, category_description, category_order, category_icon           FROM         category         WHERE        category.category_id=1

SELECT         page_id, page_key, page_title, page_content, page_create_time, page_update_time,     page_view_count, page_comment_count, page_status       FROM  page
SELECT count(0) FROM article WHERE article.article_status = 1 AND article.article_id IN (SELECT article_category_ref.article_id FROM article_category_ref WHERE article_category_ref.category_id = 1) AND 1 = 1
SELECT        article.*        FROM        article         WHERE article.article_status = 1 AND                                                                article.article_id IN (                SELECT article_category_ref.article_id FROM article_category_ref                WHERE article_category_ref.category_id = 1                ) AND                                    1 = 1         ORDER BY `article`.`article_order` DESC, `article`.`article_id` DESC limit 0,10
