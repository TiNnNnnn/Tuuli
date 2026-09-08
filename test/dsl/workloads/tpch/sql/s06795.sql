SELECT
  n.n_name AS nation_name,
  r.r_name AS region_name,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  COUNT(DISTINCT c.c_custkey) AS total_customers
FROM customer AS c
JOIN orders AS o
  ON c.c_custkey = o.o_custkey
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  l.l_shipdate >= CAST('1997-01-01' AS DATE)
  AND l.l_shipdate < CAST('1997-12-31' AS DATE)
  AND r.r_name = 'Europe'
GROUP BY
  n.n_name,
  r.r_name
ORDER BY
  total_revenue DESC,
  total_orders DESC
LIMIT 10;
