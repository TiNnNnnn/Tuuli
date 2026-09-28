SELECT i FROM quant_outer
WHERE v = ANY (SELECT v FROM quant_inner GROUP BY v)
ORDER BY i;
