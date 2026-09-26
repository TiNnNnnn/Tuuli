SELECT i FROM quant_outer
WHERE i > 1
  AND i = ANY (SELECT v FROM quant_inner WHERE v > 10)
ORDER BY i;
