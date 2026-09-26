SELECT i FROM quant_outer
WHERE v = ANY (SELECT i FROM quant_outer)
ORDER BY i;
