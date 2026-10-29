SELECT q.i, q.lower_value, q.j > 1 AS upper_value
FROM (SELECT i, j, -i AS lower_value FROM overflow_input) q
ORDER BY q.i NULLS FIRST;
