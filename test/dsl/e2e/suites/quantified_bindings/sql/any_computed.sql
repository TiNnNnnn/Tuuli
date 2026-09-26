SELECT i, v = ANY (SELECT NOT selected FROM quant_wide) FROM quant_outer ORDER BY i;
