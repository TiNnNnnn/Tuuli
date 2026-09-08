SELECT
  COUNT(*) AS total_orders,
  SUM(o.o_totalprice) AS total_revenue,
  AVG(l.l_extendedprice) AS avg_lineitem_price,
  COUNT(DISTINCT c.c_custkey) AS total_customers
FROM orders AS o
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  o.o_orderstatus = 'O'
  AND l.l_shipdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
GROUP BY
  o.o_orderpriority
ORDER BY
  total_revenue DESC;
