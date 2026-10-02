SELECT l.a, l.b FROM dsl_eq_pair_left l
WHERE l.a > 0 AND EXISTS (
    SELECT * FROM dsl_eq_pair_right r WHERE r.a <> l.a AND 1/(r.b-r.b)>0)
ORDER BY l.a NULLS FIRST, l.b NULLS FIRST;
