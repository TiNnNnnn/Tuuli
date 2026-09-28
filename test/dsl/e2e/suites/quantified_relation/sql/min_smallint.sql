SELECT i FROM quant_outer
WHERE v = ANY (SELECT min(s)::int FROM quant_extrema)
ORDER BY i;
