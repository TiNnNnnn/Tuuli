SELECT 'all' AS tag, i FROM quant_outer o
WHERE v = ALL (SELECT q.v FROM quant_inner q WHERE q.v IS NOT DISTINCT FROM o.v)
UNION ALL
SELECT 'any', i FROM quant_outer o
WHERE v = ANY (SELECT q.v FROM quant_inner q WHERE q.v IS NOT DISTINCT FROM o.v)
ORDER BY tag, i;
