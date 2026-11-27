SELECT a.k, x.k, x.maximum, x.group_key
FROM (VALUES (1), (2)) AS a(k)
LEFT JOIN LATERAL (
    SELECT o.k, q.maximum, q.group_key
    FROM dsl_eq_left AS o LEFT JOIN LATERAL (
        SELECT max(i.v) AS maximum, max(i.g) AS group_key
        FROM dsl_agg_outer AS i WHERE i.g = o.k HAVING max(i.v) > 7
    ) AS q ON true
    WHERE o.k >= a.k ORDER BY o.k LIMIT 3
) AS x ON true
ORDER BY a.k, x.k;
