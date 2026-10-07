SELECT scenario, value FROM (
    SELECT 'all_min' AS scenario, o.case_id AS value FROM dsl_notin_outer o
    WHERE o.x <> ALL (
        SELECT min(DISTINCT i.y) FROM dsl_notin_inner i
        WHERE i.set_id = o.set_id GROUP BY i.set_id)
    UNION ALL
    SELECT 'any_max', o.case_id FROM dsl_notin_outer o
    WHERE o.x < ANY (
        SELECT max(DISTINCT i.y) FROM dsl_notin_inner i
        WHERE i.set_id = o.set_id GROUP BY i.set_id)
    UNION ALL
    SELECT 'all_empty_scalar', o.case_id FROM dsl_notin_outer o
    WHERE o.x <> ALL (
        SELECT min(DISTINCT i.y) FROM dsl_notin_inner i WHERE i.set_id = 99)
    UNION ALL
    SELECT 'any_null_scalar', o.case_id FROM dsl_notin_outer o
    WHERE o.x < ANY (
        SELECT max(DISTINCT i.y) FROM dsl_notin_inner i WHERE i.y IS NULL)
) cases
ORDER BY scenario, value;
