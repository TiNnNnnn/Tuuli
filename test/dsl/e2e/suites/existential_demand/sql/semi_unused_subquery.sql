SELECT i FROM apply_outer
WHERE EXISTS (SELECT (SELECT j FROM apply_inner) FROM apply_inner WHERE j=i AND j>0)
ORDER BY i NULLS FIRST;
