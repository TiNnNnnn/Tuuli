SELECT o.id * 100 + u.k
FROM dsl_insub_outer AS o
INNER JOIN (
    SELECT k FROM dsl_eq_left WHERE k <= 2
    UNION ALL
    SELECT k FROM dsl_eq_right WHERE k >= 2
) AS u ON o.id = u.k
ORDER BY 1;
