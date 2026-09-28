SELECT i, (SELECT selected FROM scalar_empty GROUP BY selected)
FROM scalar_outer ORDER BY i;
