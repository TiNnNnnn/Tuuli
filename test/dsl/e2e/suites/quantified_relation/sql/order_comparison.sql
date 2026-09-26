SELECT i FROM quant_outer
WHERE i > ALL (SELECT v FROM quant_equal)
ORDER BY i;
