SELECT i, v = ANY (SELECT v FROM quant_empty) FROM quant_outer ORDER BY i;
