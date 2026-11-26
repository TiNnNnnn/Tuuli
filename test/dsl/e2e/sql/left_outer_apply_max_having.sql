SELECT o.k, q.maximum, q.group_key
FROM dsl_eq_left AS o
LEFT JOIN LATERAL (
    SELECT max(i.v) AS maximum, max(i.g) AS group_key
    FROM dsl_agg_outer AS i
    WHERE i.g = o.k
    HAVING max(i.v) > 7
) AS q ON true
ORDER BY o.k;
