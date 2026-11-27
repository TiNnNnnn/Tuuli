SELECT -1 AS k, 77 AS maximum
UNION ALL
SELECT o.k, q.maximum
FROM dsl_eq_left o
LEFT JOIN LATERAL (
    SELECT max(i.v) AS maximum, max(i.g) AS group_key
    FROM dsl_agg_outer i WHERE i.g = o.k HAVING max(i.v) > 7
) q ON true
ORDER BY k, maximum;
