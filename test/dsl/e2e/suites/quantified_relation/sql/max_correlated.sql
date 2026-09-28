SELECT o.i FROM quant_outer o
WHERE o.v = ANY (SELECT max(q.v) FROM quant_inner q WHERE q.v = o.v GROUP BY q.v)
ORDER BY o.i;
