SELECT i, (SELECT selected FROM scalar_empty) = (i > 0) FROM scalar_outer ORDER BY i;
