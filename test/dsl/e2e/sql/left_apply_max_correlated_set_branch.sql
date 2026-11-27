SELECT a.k,
       (SELECT branch.maximum
        FROM (
            SELECT 0 AS k, 77 AS maximum
            UNION ALL
            SELECT o.k, q.maximum
            FROM dsl_eq_left o
            LEFT JOIN LATERAL (
                SELECT max(i.v) AS maximum, max(i.g) AS group_key
                FROM dsl_agg_outer i WHERE i.g = o.k HAVING max(i.v) > 7
            ) q ON true
        ) branch
        WHERE branch.k >= a.k
        ORDER BY branch.k, branch.maximum LIMIT 1)
FROM (VALUES (0), (1), (2), (3)) a(k) ORDER BY a.k;
