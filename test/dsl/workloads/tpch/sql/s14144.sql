SELECT
  AVG(l_extendedprice) AS avg_extended_price,
  n_name
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN nation AS n
  ON c.c_nationkey = n.n_nationkey
GROUP BY
  n_name
ORDER BY
  avg_extended_price DESC
LIMIT 10;
