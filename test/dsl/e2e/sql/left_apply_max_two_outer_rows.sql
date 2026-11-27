SELECT a.k,
       (SELECT (SELECT q.maximum
                FROM dsl_eq_left o
                LEFT JOIN LATERAL (
                    SELECT max(i.v) AS maximum, max(i.g) AS group_key
                    FROM dsl_agg_outer i WHERE i.g = o.k HAVING max(i.v) > 7
                ) q ON true
                WHERE o.k >= a.k + b.j - 1 ORDER BY o.k, q.maximum LIMIT 1)
        FROM (VALUES (1), (2)) b(j)
        WHERE b.j >= a.k ORDER BY b.j LIMIT 1)
FROM (VALUES (1), (2), (3)) a(k) ORDER BY a.k;
