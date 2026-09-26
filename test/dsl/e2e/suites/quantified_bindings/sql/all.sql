SELECT i, v = ALL (SELECT v FROM quant_inner) FROM quant_outer ORDER BY i;
