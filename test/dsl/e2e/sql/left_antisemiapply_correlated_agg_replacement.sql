SELECT o.k
FROM dsl_eq_right AS o
WHERE NOT EXISTS (
    SELECT count(*)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.k
    GROUP BY i.g
    HAVING i.g = i.g)
ORDER BY o.k;
