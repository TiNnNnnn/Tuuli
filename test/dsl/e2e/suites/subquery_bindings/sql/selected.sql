SELECT i, (SELECT selected FROM scalar_inner WHERE i = 2) FROM scalar_outer ORDER BY i;
