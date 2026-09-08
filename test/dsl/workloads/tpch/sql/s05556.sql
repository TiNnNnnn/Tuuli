SELECT
  n.n_name AS nation_name,
  r.r_name AS region_name,
  COUNT(DISTINCT c.c_custkey) AS total_customers,
  SUM(o.o_totalprice) AS total_sales,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  AVG(o.o_totalprice) AS avg_order_value,
  SUM(CASE WHEN l.l_returnflag = 'R' THEN l.l_quantity ELSE 0 END) AS total_returns,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue
FROM nation AS n
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
JOIN partsupp AS ps
  ON s.s_suppkey = ps.ps_suppkey
JOIN part AS p
  ON ps.ps_partkey = p.p_partkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  o.o_orderdate >= CAST('1997-01-01' AS DATE)
  AND o.o_orderdate < CAST('1998-01-01' AS DATE)
GROUP BY
  n.n_name,
  r.r_name
ORDER BY
  total_sales DESC,
  total_customers DESC
LIMIT 100;
