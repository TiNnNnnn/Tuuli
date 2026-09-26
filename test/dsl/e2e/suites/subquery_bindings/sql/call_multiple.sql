SELECT i, (SELECT selected FROM scalar_inner) = (i > 0) FROM scalar_outer ORDER BY i;
