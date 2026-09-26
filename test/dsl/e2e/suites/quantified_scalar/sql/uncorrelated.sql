SELECT 'all' AS tag, i FROM quant_outer WHERE v = ALL (SELECT v FROM quant_equal)
UNION ALL
SELECT 'any', i FROM quant_outer WHERE v = ANY (SELECT v FROM quant_inner)
ORDER BY tag, i;
