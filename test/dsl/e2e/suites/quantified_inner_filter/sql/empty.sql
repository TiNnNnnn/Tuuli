SELECT 'all' AS tag, i FROM quant_outer o WHERE v = ALL (SELECT q.v FROM quant_empty q WHERE q.a OR q.b)
UNION ALL
SELECT 'any', i FROM quant_outer o WHERE v = ANY (SELECT q.v FROM quant_empty q WHERE q.a OR q.b)
ORDER BY tag, i;
