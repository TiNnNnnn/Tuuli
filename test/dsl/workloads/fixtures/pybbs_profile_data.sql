-- Deterministic nonempty mechanism fixture, not production-distribution evidence.
INSERT INTO "user" (id, username, score, in_time, token, email_notification, active)
SELECT i, 'user_' || i, 0, TIMESTAMP '2020-01-01', 'token_' || i, 0, 1
FROM generate_series(1, 1000) AS g(i);
INSERT INTO topic (id, title, in_time, user_id, comment_count, collect_count, "view", top, good)
SELECT i, 'topic_' || i, TIMESTAMP '2020-01-01', 1 + (i % 1000), 0, 0, 0, 0, 0
FROM generate_series(1, 5000) AS g(i);
ANALYZE;
