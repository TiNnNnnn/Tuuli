SELECT i FROM quant_outer
WHERE v::bigint = ANY (SELECT max(b) FROM quant_extrema)
ORDER BY i;
