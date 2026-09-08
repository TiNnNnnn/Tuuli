SELECT
  p.p_name,
  s.s_name,
  c.c_name,
  n.n_name,
  r.r_name,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  STRING_AGG(DISTINCT SUBSTRING(l.l_comment FROM 1 FOR 30), ', ') AS short_comments
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
  r.r_name LIKE 'N%' AND o.o_orderdate BETWEEN '1996-01-01' AND '1997-12-31'
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name,
  n.n_name,
  r.r_name
HAVING
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) > 10000
ORDER BY
  total_revenue DESC;
