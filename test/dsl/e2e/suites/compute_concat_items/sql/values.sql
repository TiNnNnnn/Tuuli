SELECT q.i, q.lower_a, q.lower_b, q.j > 1 AS upper_a, q.j IS NULL AS upper_b
FROM (
    SELECT i, j, i > 0 AS lower_a, i > 2 AS lower_b
    FROM (VALUES (NULL::integer, 2), (0, 1), (1, 1),
                 (2, NULL::integer), (2, NULL::integer), (3, 2)) AS v(i, j)
) q
ORDER BY q.i NULLS FIRST;
