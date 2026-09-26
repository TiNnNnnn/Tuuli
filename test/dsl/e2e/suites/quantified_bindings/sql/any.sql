SELECT i, v = ANY (SELECT v FROM quant_inner) FROM quant_outer ORDER BY i;
