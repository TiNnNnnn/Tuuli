SELECT i, v = ANY (SELECT selected FROM quant_wide) FROM quant_outer ORDER BY i;
