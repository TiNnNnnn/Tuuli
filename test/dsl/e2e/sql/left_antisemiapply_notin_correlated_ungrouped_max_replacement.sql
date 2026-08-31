SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.x NOT IN (
    SELECT max(i.y)
    FROM dsl_notin_inner AS i
    WHERE i.set_id = o.set_id
    GROUP BY i.y)
ORDER BY o.case_id;
