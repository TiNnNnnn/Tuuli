SELECT k FROM dsl_eq_left
INTERSECT
SELECT k FROM dsl_eq_right
ORDER BY k;
