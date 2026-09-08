SELECT
  p.p_partkey,
  p.p_name,
  s.s_name,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN lineitem AS l
  ON ps.ps_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  o.o_orderdate >= CAST('1995-01-01' AS DATE)
  AND o.o_orderdate < CAST('1996-01-01' AS DATE)
GROUP BY
  p.p_partkey,
  p.p_name,
  s.s_name
ORDER BY
  revenue DESC
LIMIT 10;
