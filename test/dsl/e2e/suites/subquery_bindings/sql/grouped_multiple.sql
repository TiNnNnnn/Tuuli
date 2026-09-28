SELECT i, (SELECT selected FROM scalar_inner GROUP BY selected)
FROM scalar_outer ORDER BY i;
