SELECT i, (SELECT NOT selected FROM scalar_inner WHERE i = 3) FROM scalar_outer ORDER BY i;
