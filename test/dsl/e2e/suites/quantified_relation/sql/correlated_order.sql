SELECT o.i FROM quant_outer o
WHERE o.i > ALL (
  SELECT q.v FROM quant_inner q WHERE q.v = o.i
)
ORDER BY o.i;
