SELECT o.i, (SELECT s.selected FROM
  (SELECT selected, generate_series(1, 0) AS unused FROM scalar_inner WHERE i = 2) s)
FROM scalar_outer o ORDER BY o.i;
