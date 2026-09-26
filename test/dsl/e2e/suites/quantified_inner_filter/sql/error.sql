SELECT 'all' AS tag, i FROM quant_outer o WHERE v = ALL (SELECT q.v FROM quant_error q WHERE q.a OR 10 / q.v > 0)
UNION ALL
SELECT 'any', i FROM quant_outer o WHERE v = ANY (SELECT q.v FROM quant_error q WHERE q.a OR 10 / q.v > 0)
ORDER BY tag, i;
