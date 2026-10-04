SELECT o.i, (
  SELECT count(*) FROM apply_pairs AS m WHERE NOT EXISTS (
    SELECT * FROM (
      SELECT j FROM apply_inner WHERE j <> m.k AND j <> o.i AND j <> 0
      ORDER BY j LIMIT 1
    ) AS bounded WHERE j > 0
  )
) FROM apply_outer AS o ORDER BY o.i NULLS FIRST;
