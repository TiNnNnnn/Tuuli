-- Synthetic lead domain with overlapping access predicates and nullable fields.
INSERT INTO leads (id, user_id, assigned_to, first_name, last_name, access, created_at, rating, do_not_call)
SELECT i, CASE WHEN i % 3 = 0 THEN 980 ELSE 981 END,
       CASE WHEN i % 5 = 0 THEN 980 END, 'lead_' || i, 'last_' || i,
       CASE WHEN i % 2 = 0 THEN 'Public' ELSE 'Private' END,
       TIMESTAMP '2020-01-01' + i * INTERVAL '1 second', 0, 0
FROM generate_series(1, 1024) AS g(i);
ANALYZE;
