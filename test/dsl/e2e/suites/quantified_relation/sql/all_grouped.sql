SELECT i FROM quant_outer
WHERE v = ALL (SELECT v FROM quant_equal GROUP BY v)
ORDER BY i;
