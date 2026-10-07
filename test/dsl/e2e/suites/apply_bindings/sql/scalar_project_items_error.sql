SELECT (SELECT min(i) FROM binding_right) AS s, 10 / i AS x, i * 2 AS y
FROM binding_left ORDER BY x NULLS FIRST;
