SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.x <> ALL (
    SELECT DISTINCT i.y
    FROM dsl_notin_inner AS i
    WHERE i.set_id = o.set_id)
ORDER BY o.case_id;
