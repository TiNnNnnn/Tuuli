SELECT i FROM quant_outer
WHERE COALESCE(v, 0) = ANY (SELECT v FROM quant_inner)
ORDER BY i;
