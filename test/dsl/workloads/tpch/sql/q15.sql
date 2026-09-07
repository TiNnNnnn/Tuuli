WITH revenue0(supplier_no, total_revenue) AS (
  SELECT
    l_suppkey,
    SUM(l_extendedprice * (
      1 - l_discount
    ))
  FROM lineitem
  WHERE
    l_shipdate >= CAST('1994-02-01' AS DATE)
    AND l_shipdate < CAST('1994-02-01' AS DATE) + INTERVAL '3 MONTH'
  GROUP BY
    l_suppkey
)
SELECT
  s_suppkey,
  s_name,
  s_address,
  s_phone,
  total_revenue
FROM supplier, revenue0
WHERE
  s_suppkey = supplier_no
  AND total_revenue = (
    SELECT
      MAX(total_revenue)
    FROM revenue0
  )
ORDER BY
  s_suppkey;
