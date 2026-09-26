SELECT i, v = ALL (SELECT v FROM quant_inner WHERE v IS NOT DISTINCT FROM quant_outer.v)
FROM quant_outer ORDER BY i;
