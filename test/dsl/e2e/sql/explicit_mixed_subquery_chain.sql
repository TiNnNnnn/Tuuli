SELECT scenario, case_id FROM (
    SELECT 'exists_first' AS scenario, o.case_id
    FROM dsl_notin_outer o
    WHERE EXISTS (SELECT 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
      AND o.x < ANY (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
      AND o.x <> ALL (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
    UNION ALL
    SELECT 'exists_last', o.case_id
    FROM dsl_notin_outer o
    WHERE o.x <> ALL (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
      AND o.x < ANY (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
      AND EXISTS (SELECT 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
    UNION ALL
    SELECT 'not_exists', o.case_id
    FROM dsl_notin_outer o
    WHERE o.x < ANY (SELECT i.y FROM dsl_notin_inner i)
      AND o.x <> ALL (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
      AND NOT EXISTS (SELECT 1 FROM dsl_notin_inner i
                      WHERE i.set_id = o.set_id AND i.y = o.x)
) cases
ORDER BY scenario, case_id;
