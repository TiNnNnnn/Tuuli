SELECT
  l_orderkey,
  SUM(l_extendedprice * (
    1 - l_discount
  )) AS total_revenue
FROM lineitem
WHERE
  l_shipdate >= CAST('1995-01-01' AS DATE)
GROUP BY
  l_orderkey
ORDER BY
  total_revenue DESC
LIMIT 10;
