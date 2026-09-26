SELECT i, v = ALL (SELECT selected FROM quant_wide) FROM quant_outer ORDER BY i;
