SELECT (SELECT min(i) FROM binding_right)::bigint AS s,
       i IS NULL AS x, i IS NOT NULL AS y
FROM binding_left ORDER BY i NULLS FIRST;
