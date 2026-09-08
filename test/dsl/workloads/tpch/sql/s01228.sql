WITH SupplierSales AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    COUNT(DISTINCT o.o_orderkey) AS total_orders
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN lineitem AS l
    ON ps.ps_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate < CAST('1997-12-31' AS DATE)
  GROUP BY
    s.s_suppkey,
    s.s_name
), HighValueSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal
  FROM supplier AS s
  WHERE
    s.s_acctbal > (
      SELECT
        AVG(s2.s_acctbal)
      FROM supplier AS s2
    )
), RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    ss.total_sales,
    ss.total_orders,
    RANK() OVER (ORDER BY ss.total_sales DESC) AS sales_rank
  FROM SupplierSales AS ss
  JOIN HighValueSuppliers AS s
    ON ss.s_suppkey = s.s_suppkey
)
SELECT
  r.r_name,
  rs.s_suppkey,
  rs.s_name,
  COALESCE(rs.total_sales, 0) AS total_sales,
  COALESCE(rs.total_orders, 0) AS total_orders,
  CASE
    WHEN rs.total_sales IS NULL
    THEN 'No Sales'
    WHEN rs.total_sales < 10000
    THEN 'Low Sales'
    WHEN rs.total_sales >= 10000 AND rs.total_sales < 50000
    THEN 'Medium Sales'
    ELSE 'High Sales'
  END AS sales_category
FROM region AS r
LEFT JOIN nation AS n
  ON r.r_regionkey = n.n_regionkey
LEFT JOIN (
  SELECT DISTINCT
    s_nationkey
  FROM supplier
) AS ns
  ON n.n_nationkey = ns.s_nationkey
LEFT JOIN RankedSuppliers AS rs
  ON ns.s_nationkey = rs.s_suppkey
WHERE
  r.r_name LIKE 'N%'
ORDER BY
  sales_category,
  total_sales DESC;
