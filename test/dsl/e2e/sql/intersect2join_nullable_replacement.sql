SELECT a, b FROM dsl_eq_pair_left
INTERSECT
SELECT a, b FROM dsl_eq_pair_right
ORDER BY a NULLS LAST, b NULLS LAST;
