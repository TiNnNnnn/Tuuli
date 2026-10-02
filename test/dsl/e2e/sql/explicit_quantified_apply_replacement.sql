SELECT scenario, value FROM (
    SELECT 'all' AS scenario, o.case_id AS value FROM dsl_notin_outer o
    WHERE o.case_id > 0
      AND o.x <> ALL (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
    UNION ALL
    SELECT 'any', o.case_id FROM dsl_notin_outer o
    WHERE o.x < ANY (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
    UNION ALL
    SELECT 'any_compound', o.case_id FROM dsl_notin_outer o
    WHERE COALESCE(o.x, 0) = ANY (SELECT r.a FROM dsl_eq_pair_right r)
    UNION ALL
    SELECT 'bag', l.a FROM dsl_eq_pair_left l
    WHERE l.a = ANY (SELECT r.a FROM dsl_eq_pair_right r)
) cases
ORDER BY scenario, value;
