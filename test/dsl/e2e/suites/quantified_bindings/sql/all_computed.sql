SELECT i, v = ALL (SELECT NOT selected FROM quant_wide) FROM quant_outer ORDER BY i;
