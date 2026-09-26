SELECT i, CASE WHEN i < 0 THEN (SELECT selected FROM scalar_inner) ELSE false END FROM scalar_outer ORDER BY i;
