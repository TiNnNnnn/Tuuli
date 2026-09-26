SELECT i FROM quant_outer
WHERE v = ANY (SELECT v / (v - v) FROM quant_inner)
ORDER BY i;
