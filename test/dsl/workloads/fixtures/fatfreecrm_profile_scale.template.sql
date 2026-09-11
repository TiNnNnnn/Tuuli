-- Controlled synthetic data scale; not an estimated-cardinality injection.
INSERT INTO contacts (id, user_id, assigned_to, first_name, last_name, access,
                      email, do_not_call, deleted_at, created_at)
SELECT i, 547, 548, 'first_' || i, 'last_' || i, 'Public',
       CASE WHEN i % 10 < 3 THEN 'page_sanford@kuvalisraynor.biz' ELSE 'other@example.org' END,
       0, TIMESTAMP '2020-01-01', TIMESTAMP '2020-01-01' + i * INTERVAL '1 second'
FROM generate_series(1, ${rows}) AS g(i);
ANALYZE;
