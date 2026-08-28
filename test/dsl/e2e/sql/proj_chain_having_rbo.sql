SELECT q.k
FROM (
    SELECT k
    FROM dsl_eq_left
    GROUP BY k
    HAVING k > 1
) AS q
GROUP BY q.k
ORDER BY q.k;
