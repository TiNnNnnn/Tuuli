SELECT q.i, q.lower_a, q.lower_b, 1 / (q.i - q.i) AS upper_a
FROM (
    SELECT i, i > 0 AS lower_a, i > 2 AS lower_b FROM compute_input
) q
ORDER BY q.i NULLS FIRST;
