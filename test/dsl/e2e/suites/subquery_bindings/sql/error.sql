SELECT i, (SELECT 1 / i > 0 FROM scalar_error) FROM scalar_outer ORDER BY i;
