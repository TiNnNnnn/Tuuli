SELECT i FROM apply_outer
WHERE i>0 AND EXISTS (SELECT * FROM apply_inner WHERE j<>i AND 1/(j-j)>0)
ORDER BY i NULLS FIRST;
