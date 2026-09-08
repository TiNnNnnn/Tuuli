SELECT
  p.p_name,
  s.s_name,
  n.n_name,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  AVG(l.l_extendedprice * (
    1 - l.l_discount
  )) AS avg_order_value
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  n.n_name LIKE 'A%'
  AND l.l_shipdate BETWEEN CAST('1996-01-01' AS DATE) AND CAST('1996-12-31' AS DATE)
  AND ps.ps_availqty > 10
GROUP BY
  p.p_name,
  s.s_name,
  n.n_name
ORDER BY
  total_orders DESC,
  avg_order_value DESC
LIMIT 10;
