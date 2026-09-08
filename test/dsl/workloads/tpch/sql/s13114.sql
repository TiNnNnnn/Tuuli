SELECT
  n_name AS nation,
  SUM(l_extendedprice * (
    1 - l_discount
  )) AS total_revenue
FROM lineitem
JOIN orders
  ON l_orderkey = o_orderkey
JOIN customer
  ON o_custkey = c_custkey
JOIN nation
  ON c_nationkey = n_nationkey
WHERE
  o_orderdate >= CAST('1996-01-01' AS DATE)
  AND o_orderdate < CAST('1997-01-01' AS DATE)
GROUP BY
  n_name
ORDER BY
  total_revenue DESC;
