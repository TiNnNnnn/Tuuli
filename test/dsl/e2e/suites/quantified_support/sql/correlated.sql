SELECT 'all' AS tag, i FROM quant_outer o WHERE v <= ALL (SELECT DISTINCT q.v FROM quant_inner q WHERE q.v >= o.v)
UNION ALL
SELECT 'any', i FROM quant_outer o WHERE v < ANY (SELECT DISTINCT q.v FROM quant_inner q WHERE q.v >= o.v)
ORDER BY tag, i;
