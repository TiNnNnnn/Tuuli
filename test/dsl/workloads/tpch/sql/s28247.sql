SELECT
  p.p_name,
  s.s_name,
  c.c_name,
  n.n_name,
  r.r_name,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  SUM(l.l_quantity) AS total_quantity,
  AVG(l.l_extendedprice) AS avg_price,
  STRING_AGG(DISTINCT p.p_comment, '; ') AS aggregated_comments
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
  p.p_type LIKE '%brass%' AND l.l_shipdate BETWEEN '1997-01-01' AND '1997-12-31'
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name,
  n.n_name,
  r.r_name
ORDER BY
  total_orders DESC,
  total_quantity DESC;
