SELECT i FROM quant_outer
WHERE v = ANY (SELECT max(v) FROM quant_inner GROUP BY v)
ORDER BY i;
