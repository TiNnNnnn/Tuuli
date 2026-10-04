SELECT i FROM apply_outer WHERE NOT EXISTS (
  SELECT * FROM (
    SELECT j FROM apply_inner WHERE j <> i AND j <> 0 ORDER BY j LIMIT 1
  ) AS bounded WHERE j > 0
) ORDER BY i NULLS FIRST;
