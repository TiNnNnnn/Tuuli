SELECT ((SELECT min(i) FROM binding_empty) + i) * 2 AS s, i + 1 AS x, i * 2 AS y
FROM binding_left ORDER BY x NULLS FIRST;
