SELECT ((SELECT min(i) FROM binding_right) IS NULL) OR (i IS NOT NULL) AS b,
       i IS NULL AS x
FROM binding_left ORDER BY i NULLS FIRST;
