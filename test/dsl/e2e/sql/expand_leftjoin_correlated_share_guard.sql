SELECT o.k,
       (SELECT count(r.k)
        FROM dsl_loj_outer AS l
        LEFT JOIN (SELECT i.k FROM dsl_loj_inner AS i
                   WHERE i.k = o.k LIMIT 1) AS r ON r.k = l.k)
FROM dsl_loj_outer AS o
ORDER BY o.k;
