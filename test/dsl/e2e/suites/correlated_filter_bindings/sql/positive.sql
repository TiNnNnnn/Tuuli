SELECT i FROM correlation_outer
WHERE EXISTS (SELECT 1 FROM correlation_inner WHERE j = i)
ORDER BY i NULLS FIRST;
