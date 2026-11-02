SELECT o.k
FROM dsl_eq_right AS o
WHERE NOT EXISTS (
    SELECT min(i.v)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.k
    GROUP BY i.g
    HAVING min(i.v) = 10)
ORDER BY o.k;
