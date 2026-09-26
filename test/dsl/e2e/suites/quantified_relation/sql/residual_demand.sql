SELECT i FROM quant_outer
WHERE i / (i - i) > 0
  AND i = ANY (SELECT v FROM quant_inner WHERE v > 10)
ORDER BY i;
