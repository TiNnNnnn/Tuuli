SELECT i, CASE WHEN i > 0 THEN (SELECT i FROM scalar_inner WHERE i = 2) ELSE 0 END FROM scalar_outer ORDER BY i;
