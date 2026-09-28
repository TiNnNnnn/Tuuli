-- The selected int key is safe, but every grouping key must be checked.
SELECT i FROM quant_outer
WHERE v = ANY (SELECT v FROM quant_keys GROUP BY v, k)
ORDER BY i;
