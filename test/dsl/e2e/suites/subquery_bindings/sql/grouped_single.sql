SELECT i, (SELECT selected FROM scalar_inner WHERE i = 2 GROUP BY selected)
FROM scalar_outer ORDER BY i;
