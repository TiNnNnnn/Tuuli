SELECT
  n.n_name,
  o.o_orderstatus,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_sales
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN nation AS n
  ON c.c_nationkey = n.n_nationkey
GROUP BY
  n.n_name,
  o.o_orderstatus
HAVING
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) > 10000
ORDER BY
  total_sales DESC;
