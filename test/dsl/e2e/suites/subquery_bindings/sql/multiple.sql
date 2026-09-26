SELECT i, (SELECT selected FROM scalar_inner) FROM scalar_outer ORDER BY i;
