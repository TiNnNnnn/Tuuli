SELECT i + 10 AS x, j, j + 100 AS y FROM projection_input
WHERE i > 1 OR j > 1
ORDER BY x NULLS FIRST, j NULLS FIRST;
