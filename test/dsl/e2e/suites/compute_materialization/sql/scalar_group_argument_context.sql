SELECT (SELECT min(i::bigint) FROM binding_right) AS s,
       i IS NULL AS x, i IS NOT NULL AS y
FROM binding_left ORDER BY i NULLS FIRST;
