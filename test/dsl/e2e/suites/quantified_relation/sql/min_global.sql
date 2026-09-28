SELECT i FROM quant_outer
WHERE v = ALL (SELECT min(v) FROM quant_inner)
ORDER BY i;
