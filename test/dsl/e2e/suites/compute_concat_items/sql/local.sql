SELECT q.i, q.lower_a, q.lower_b, q.j > 1 AS upper_a, q.j IS NULL AS upper_b
FROM (
    SELECT i, j, i > 0 AS lower_a, i > 2 AS lower_b FROM compute_input
) q
ORDER BY q.i NULLS FIRST;
