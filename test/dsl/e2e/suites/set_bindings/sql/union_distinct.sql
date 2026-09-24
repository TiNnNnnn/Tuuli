-- Right input uses the reverse physical column order.
SELECT a, COALESCE(b,-1) FROM (
  SELECT a,b FROM set_left WHERE a > 0
  UNION
  SELECT b,a FROM set_right WHERE b > 0
) s ORDER BY 1,2;
