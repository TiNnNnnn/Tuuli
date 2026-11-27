SELECT a.k, a.k = 0 OR (SELECT q.maximum
    FROM dsl_eq_left o
    LEFT JOIN LATERAL (
        SELECT max(i.v) AS maximum, max(i.g) AS group_key
        FROM dsl_agg_outer i WHERE i.g = o.k HAVING max(i.v) > 7
    ) q ON true
    WHERE o.k >= a.k ORDER BY o.k, q.maximum LIMIT 1) > 7 AS hit
FROM (VALUES (0), (1), (2), (3)) a(k) ORDER BY a.k;
