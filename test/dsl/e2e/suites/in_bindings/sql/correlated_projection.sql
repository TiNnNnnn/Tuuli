SELECT i FROM in_outer o
WHERE i IN (SELECT j FROM in_wide WHERE k > o.i)
ORDER BY i NULLS FIRST;
