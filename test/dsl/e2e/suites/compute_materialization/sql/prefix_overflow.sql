SELECT i + 2147483647 AS x, (SELECT min(i) FROM binding_right) AS s
FROM binding_left;
