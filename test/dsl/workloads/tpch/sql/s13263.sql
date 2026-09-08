SELECT
  n.n_name,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN supplier AS s
  ON l.l_suppkey = s.s_suppkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey AND s.s_suppkey = ps.ps_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
WHERE
  o.o_orderdate >= CAST('1997-01-01' AS DATE)
  AND o.o_orderdate < CAST('1997-02-01' AS DATE)
GROUP BY
  n.n_name
ORDER BY
  total_revenue DESC;
