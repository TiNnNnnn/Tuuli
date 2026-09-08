SELECT
  p.p_name AS part_name,
  s.s_name AS supplier_name,
  c.c_name AS customer_name,
  o.o_orderkey AS order_key,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  STRING_AGG(DISTINCT n.n_name, ', ') AS nation_names
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN lineitem AS l
  ON ps.ps_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
WHERE
  o.o_orderdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name,
  o.o_orderkey
HAVING
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) > 1000.00
ORDER BY
  total_revenue DESC;
