SELECT q.i, q.lower_a, q.lower_b, q.j > 1 AS upper_a
FROM (
    SELECT i, j, i > 0 AS lower_a, i > 2 AS lower_b
    FROM compute_input WHERE i < 0
) q
ORDER BY q.i;
