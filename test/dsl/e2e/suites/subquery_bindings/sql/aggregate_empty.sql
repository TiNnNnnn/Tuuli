SELECT i, CASE WHEN i > 0 THEN (SELECT max(i) FROM scalar_inner WHERE i < 0) ELSE 0 END
FROM scalar_outer ORDER BY i;
