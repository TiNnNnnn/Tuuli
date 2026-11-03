SELECT o.k
FROM dsl_eq_right AS o
WHERE NOT EXISTS (
    SELECT min(i.v)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.k
    GROUP BY i.v
    HAVING i.v = i.v)
ORDER BY o.k;
