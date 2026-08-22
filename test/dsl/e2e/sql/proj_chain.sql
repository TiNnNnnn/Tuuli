SELECT q.k
FROM (
    SELECT k FROM dsl_eq_left GROUP BY k
) AS q
GROUP BY q.k
ORDER BY q.k;

