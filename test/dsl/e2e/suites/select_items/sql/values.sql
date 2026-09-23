SELECT i > 1 AS flag, i + 10 AS x, label, j > 1 AS other
FROM select_input
ORDER BY x NULLS FIRST, label;
