SELECT i FROM quant_outer
WHERE v = ANY (SELECT v / (v - v) FROM quant_inner GROUP BY v / (v - v))
ORDER BY i;
