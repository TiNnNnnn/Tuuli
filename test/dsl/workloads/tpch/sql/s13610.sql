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
JOIN nation AS n
  ON c.c_nationkey = n.n_nationkey
WHERE
  l.l_shipdate >= CAST('1997-01-01' AS DATE)
  AND l.l_shipdate < CAST('1997-12-31' AS DATE)
GROUP BY
  n.n_name
ORDER BY
  total_revenue DESC
LIMIT 10;
