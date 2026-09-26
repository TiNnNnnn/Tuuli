SELECT EXISTS (SELECT 1 FROM exists_inner WHERE 1 / (j - j) > 0)
FROM exists_outer ORDER BY i NULLS FIRST;
