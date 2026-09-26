SELECT i FROM in_outer o
WHERE i IN (SELECT j FROM in_inner WHERE j >= o.i)
ORDER BY i NULLS FIRST;
