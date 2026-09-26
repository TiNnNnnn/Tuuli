SELECT i, (SELECT selected FROM scalar_inner WHERE i = 1) FROM scalar_outer ORDER BY i;
