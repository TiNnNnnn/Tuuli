SELECT COUNT(0) FROM dsl_residual_outer AS a
WHERE a.status=1 AND a.id IN (
    SELECT b.id FROM dsl_residual_inner AS b WHERE b.tag=1);
