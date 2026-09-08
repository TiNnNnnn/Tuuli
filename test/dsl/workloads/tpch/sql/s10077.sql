SELECT
  p.p_partkey,
  p.p_name,
  s.s_name,
  o.o_orderkey,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
GROUP BY
  p.p_partkey,
  p.p_name,
  s.s_name,
  o.o_orderkey
ORDER BY
  revenue DESC
LIMIT 100;
