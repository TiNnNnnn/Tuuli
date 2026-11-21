SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.x NOT IN (
    SELECT max(i.y)
    FROM (
        SELECT i0.set_id, CASE WHEN i0.y > o.set_id THEN i0.y END AS y
        FROM dsl_notin_inner AS i0
        OFFSET 0
    ) AS i
    WHERE i.set_id = o.set_id
    GROUP BY i.y)
ORDER BY o.case_id;
