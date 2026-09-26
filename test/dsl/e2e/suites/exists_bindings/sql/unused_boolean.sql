SELECT i, EXISTS (SELECT 1/(j-j) FROM exists_inner WHERE j > 0) AS b FROM exists_outer ORDER BY i NULLS FIRST;
