SELECT
  s.s_name AS supplier_name,
  p.p_name AS part_name,
  SUBSTRING(p.p_comment FROM 1 FOR 20) AS short_comment,
  COUNT(o.o_orderkey) AS order_count,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  r.r_name AS region_name
FROM supplier AS s
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
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  p.p_size > 10 AND s.s_acctbal > 5000 AND o.o_orderstatus = 'O'
GROUP BY
  s.s_name,
  p.p_name,
  short_comment,
  r.r_name
HAVING
  COUNT(o.o_orderkey) > 5
ORDER BY
  total_revenue DESC
LIMIT 10;
