SELECT i FROM quant_outer
WHERE 1 = ANY (SELECT v FROM quant_inner)
ORDER BY i;
