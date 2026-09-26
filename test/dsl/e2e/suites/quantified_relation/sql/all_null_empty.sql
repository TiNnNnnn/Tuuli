SELECT 'null_inner' AS tag, i FROM quant_outer
WHERE v = ALL (SELECT v FROM quant_inner)
UNION ALL
SELECT 'empty_inner', i FROM quant_outer
WHERE v = ALL (SELECT v FROM quant_empty)
ORDER BY tag, i;
