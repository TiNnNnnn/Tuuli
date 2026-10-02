SELECT l.a, l.b, l.ctid IS NOT NULL AS has_tid,
       l.tableoid = 'dsl_eq_pair_left'::regclass AS same_table
FROM dsl_eq_pair_left l
WHERE NOT EXISTS (SELECT 1 FROM dsl_eq_right r WHERE l.a > 1)
ORDER BY l.a NULLS LAST, l.b NULLS LAST;
