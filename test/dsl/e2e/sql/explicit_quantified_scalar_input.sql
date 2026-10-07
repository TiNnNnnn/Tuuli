SELECT scenario, value FROM (
    SELECT 'any_min' AS scenario, o.case_id AS value FROM dsl_notin_outer o
    WHERE o.x = ANY (
        SELECT (SELECT min(i.y) FROM dsl_notin_inner i WHERE i.set_id = j.set_id)
        FROM dsl_notin_inner j WHERE j.set_id = o.set_id)
    UNION ALL
    SELECT 'all_min', o.case_id FROM dsl_notin_outer o
    WHERE o.x <= ALL (
        SELECT (SELECT min(i.y) FROM dsl_notin_inner i WHERE i.set_id = j.set_id)
        FROM dsl_notin_inner j WHERE j.set_id = o.set_id)
) cases
ORDER BY scenario, value;
