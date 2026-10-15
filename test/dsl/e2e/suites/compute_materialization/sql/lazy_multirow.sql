SELECT i, CASE WHEN i < 0 THEN (SELECT i FROM binding_right) ELSE 0 END AS value
FROM binding_left ORDER BY i NULLS FIRST;
