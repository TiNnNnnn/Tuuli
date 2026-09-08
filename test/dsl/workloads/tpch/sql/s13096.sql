SELECT
  l_returnflag,
  l_linestatus,
  SUM(l_quantity) AS sum_qty,
  SUM(l_extendedprice) AS sum_base_price,
  SUM(l_extendedprice * (
    1 - l_discount
  )) AS sum_disc_price,
  SUM(l_extendedprice * (
    1 - l_discount
  ) * (
    1 + l_tax
  )) AS sum_charge,
  COUNT(*) AS count_order
FROM lineitem
WHERE
  l_shipdate >= CAST('1995-01-01' AS DATE)
  AND l_shipdate < CAST('1995-01-01' AS DATE) + INTERVAL '1 YEAR'
GROUP BY
  l_returnflag,
  l_linestatus
ORDER BY
  l_returnflag,
  l_linestatus;
