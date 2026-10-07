SELECT o.i, q.lower_a, q.lower_b, q.upper_a, q.upper_b
FROM compute_input o
CROSS JOIN LATERAL (
    -- Distinct outer columns prevent one captured list from masking a lost
    -- scope on the other list's explicitly reconstructed Item segment.
    SELECT q.lower_a, q.lower_b, q.j > o.j AS upper_a, q.j IS NULL AS upper_b
    FROM (
        SELECT j, i > o.i AS lower_a, i > 2 AS lower_b FROM compute_input
    ) q
    ORDER BY q.j NULLS FIRST LIMIT 1
) q
ORDER BY o.i NULLS FIRST;
