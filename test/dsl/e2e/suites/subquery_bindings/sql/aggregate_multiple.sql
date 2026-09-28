SELECT i, CASE WHEN i > 0 THEN (SELECT min(i) FROM scalar_inner GROUP BY selected) ELSE 0 END
FROM scalar_outer ORDER BY i;
