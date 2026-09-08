SELECT
  p.p_name,
  CONCAT_WS(' ', s.s_name, s.s_address) AS supplier_info,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  LEFT(p.p_comment, 20) AS short_comment,
  LEFT(n.n_name, 10) AS short_nation_name,
  UPPER(r.r_name) AS region_uppercase
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  l.l_shipdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
  AND c.c_mktsegment = 'BUILDING'
GROUP BY
  p.p_name,
  s.s_name,
  s.s_address,
  n.n_name,
  r.r_name,
  p.p_comment
HAVING
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) > 100000
ORDER BY
  total_revenue DESC,
  supplier_info ASC;
