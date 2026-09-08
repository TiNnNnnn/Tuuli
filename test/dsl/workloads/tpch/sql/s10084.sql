SELECT
  n.n_name AS nation,
  r.r_name AS region,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN nation AS n
  ON c.c_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  o.o_orderdate >= CAST('1997-01-01' AS DATE)
  AND o.o_orderdate < CAST('1998-01-01' AS DATE)
GROUP BY
  n.n_name,
  r.r_name
ORDER BY
  revenue DESC
LIMIT 10;
