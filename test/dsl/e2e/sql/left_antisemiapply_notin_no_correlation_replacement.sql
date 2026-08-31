SELECT branch || ':' || case_id
FROM (
    SELECT 'empty' AS branch, case_id
    FROM dsl_notin_outer
    WHERE case_id IN (1, 2)
      AND x NOT IN (
          SELECT y FROM dsl_notin_inner WHERE set_id = 1)

    UNION ALL

    SELECT 'match' AS branch, case_id
    FROM dsl_notin_outer
    WHERE case_id = 2
      AND x NOT IN (
          SELECT y FROM dsl_notin_inner WHERE set_id = 2)

    UNION ALL

    SELECT 'inner_null' AS branch, case_id
    FROM dsl_notin_outer
    WHERE case_id = 4
      AND x NOT IN (
          SELECT y FROM dsl_notin_inner WHERE set_id = 3)

    UNION ALL

    SELECT 'nonnull' AS branch, case_id
    FROM dsl_notin_outer
    WHERE case_id IN (5, 6)
      AND x NOT IN (
          SELECT y FROM dsl_notin_inner WHERE set_id = 4)
) AS cases
ORDER BY branch, case_id;
