SELECT i, j, label, i > 1 AS flag, i + 10 AS x
FROM compute_input ORDER BY i NULLS FIRST, label;
