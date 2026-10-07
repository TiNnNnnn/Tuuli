SELECT i + (SELECT min(i) FROM binding_right) AS v
FROM binding_left WHERE i IS NOT NULL ORDER BY v;
