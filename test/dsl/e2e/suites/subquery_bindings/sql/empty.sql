SELECT i, (SELECT selected FROM scalar_empty) FROM scalar_outer ORDER BY i;
