SELECT i FROM in_outer
WHERE i IN (SELECT coalesce(j, 0) FROM in_wide WHERE k > 0)
ORDER BY i NULLS FIRST;
