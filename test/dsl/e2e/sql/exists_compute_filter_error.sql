SELECT l.a FROM dsl_eq_pair_left l
WHERE EXISTS (SELECT r.a + 1 FROM dsl_eq_pair_right r
              WHERE 1 / (r.a - r.a) > 0)
ORDER BY l.a NULLS FIRST;
