SELECT
  n.n_name AS nation_name,
  r.r_name AS region_name,
  SUM(lp.l_extendedprice * (
    1 - lp.l_discount
  )) AS total_revenue
FROM lineitem AS lp
JOIN orders AS o
  ON lp.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN supplier AS s
  ON lp.l_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  o.o_orderdate >= CAST('1996-01-01' AS DATE)
  AND o.o_orderdate < CAST('1997-01-01' AS DATE)
GROUP BY
  n.n_name,
  r.r_name
ORDER BY
  total_revenue DESC;
