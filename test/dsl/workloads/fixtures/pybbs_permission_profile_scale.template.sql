-- Fixed-width values; 30% pid=0, no change to the original query or constraints.
INSERT INTO permission (id, name, value, pid)
SELECT i, 'name_' || lpad(i::text, 8, '0'), 'value_' || lpad(i::text, 8, '0'),
       CASE WHEN i % 10 < 3 THEN 0 ELSE 1 END
FROM generate_series(1, ${rows}) AS g(i);
ANALYZE;
