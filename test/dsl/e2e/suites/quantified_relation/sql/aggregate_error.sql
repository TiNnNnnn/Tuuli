SELECT i FROM quant_outer
WHERE v = ANY (SELECT max(v / (v - v)) FROM quant_inner)
ORDER BY i;
