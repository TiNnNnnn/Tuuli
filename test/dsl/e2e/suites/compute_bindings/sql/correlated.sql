SELECT o.i, q.flag
FROM compute_input o
CROSS JOIN LATERAL (
    SELECT i.j > o.i AS flag FROM compute_input i ORDER BY i.label LIMIT 1
) q
ORDER BY o.label;
