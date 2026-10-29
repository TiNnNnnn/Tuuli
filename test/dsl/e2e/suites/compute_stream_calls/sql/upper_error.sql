SELECT q.i, q.lower_value, 1 / (q.j - q.j) AS upper_value
FROM (SELECT i, j, -i AS lower_value FROM compute_input) q
ORDER BY q.i NULLS FIRST;
