WITH SupplierDetails AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    n.n_name AS nation_name,
    ROW_NUMBER() OVER (PARTITION BY n.n_name ORDER BY s.s_acctbal DESC) AS rank
  FROM supplier AS s
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  WHERE
    s.s_acctbal > 0
), OrderSummary AS (
  SELECT
    o.o_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    COUNT(DISTINCT l.l_partkey) AS parts_count,
    o.o_orderdate
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderdate >= CAST('1996-01-01' AS DATE)
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
), HighValueOrders AS (
  SELECT
    o.o_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    o.o_orderdate
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderstatus = 'F' AND o.o_totalprice > 1000
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
)
SELECT
  sd.s_name,
  d.r_name,
  COALESCE(os.total_sales, 0) AS order_sales,
  COALESCE(hvo.total_sales, 0) AS high_value_sales,
  sd.rank,
  CASE WHEN NOT os.total_sales IS NULL THEN 'Order Exists' ELSE 'No Order' END AS order_status
FROM SupplierDetails AS sd
LEFT JOIN region AS d
  ON sd.nation_name = d.r_name
LEFT JOIN OrderSummary AS os
  ON sd.s_suppkey = os.o_orderkey
LEFT JOIN HighValueOrders AS hvo
  ON os.o_orderkey = hvo.o_orderkey
WHERE
  sd.rank <= 5
ORDER BY
  d.r_name,
  sd.rank;
