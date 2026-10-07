SELECT q.i, q.lower_a, q.lower_b,
       NOT q.lower_a AS upper_a, q.lower_a AND q.lower_b AS upper_b
FROM (
    SELECT i, i > 0 AS lower_a, i > 2 AS lower_b FROM compute_input
) q
ORDER BY q.i NULLS FIRST;
