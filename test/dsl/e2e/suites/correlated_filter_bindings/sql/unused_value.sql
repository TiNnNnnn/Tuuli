SELECT i FROM correlation_outer
WHERE EXISTS (SELECT 1/(j-j) FROM correlation_inner WHERE j = i)
ORDER BY i NULLS FIRST;
