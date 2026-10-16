SELECT (SELECT min(i) FROM binding_right) AS s, i + 2147483647 AS x
FROM binding_left;
