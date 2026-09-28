SELECT i FROM quant_outer
WHERE v = ANY (SELECT max(v ORDER BY v) FROM quant_inner)
ORDER BY i;
