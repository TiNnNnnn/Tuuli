SELECT o.k, o.v IS NOT NULL, q.m
FROM (SELECT k, random() AS v FROM dsl_eq_left ORDER BY k LIMIT 4) AS o
LEFT JOIN LATERAL (
    SELECT max(i.v) AS m FROM dsl_agg_outer AS i
    WHERE i.g = o.k HAVING max(i.v) > 7
) AS q ON true
ORDER BY o.k;
