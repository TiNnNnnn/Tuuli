SELECT i + 1 AS x, (SELECT min(i) FROM binding_right) + 1 AS s, i * 2 AS y
FROM binding_left ORDER BY x NULLS FIRST;
