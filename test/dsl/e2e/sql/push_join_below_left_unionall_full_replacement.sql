SELECT u.k * 100 + o.v
FROM (
    SELECT k FROM dsl_eq_left WHERE k <= 2
    UNION ALL
    SELECT k FROM dsl_eq_right WHERE k >= 2
) AS u
INNER JOIN dsl_insub_outer AS o ON u.k = o.id
UNION ALL
SELECT 1000 + u.k * 100 + COALESCE(o.v, 0)
FROM (
    SELECT k FROM dsl_eq_left WHERE k <= 2
    UNION ALL
    SELECT k FROM dsl_eq_right WHERE k >= 2
) AS u
LEFT JOIN dsl_insub_outer AS o ON u.k = o.id
ORDER BY 1;
