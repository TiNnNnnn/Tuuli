-- Scale topics only; fixed 1000-user dimension, every topic has a matching user.
INSERT INTO "user" (id, username, score, in_time, token, email_notification, active)
SELECT i, 'user_' || i, 0, TIMESTAMP '2020-01-01', 'token_' || i, 0, 1
FROM generate_series(1, 1000) AS g(i);
INSERT INTO topic (id, title, in_time, user_id, comment_count, collect_count, "view", top, good)
SELECT i, 'topic_' || i, TIMESTAMP '2020-01-01', 1 + (i % 1000), 0, 0, 0, 0, 0
FROM generate_series(1, ${rows}) AS g(i);
ANALYZE;
