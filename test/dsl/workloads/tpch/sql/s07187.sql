WITH SupplierSales AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN lineitem AS l
    ON ps.ps_partkey = l.l_partkey
  GROUP BY
    s.s_suppkey,
    s.s_name
), RankedSales AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.total_sales,
    RANK() OVER (ORDER BY s.total_sales DESC) AS sales_rank
  FROM SupplierSales AS s
), TopSuppliers AS (
  SELECT
    rs.s_suppkey,
    rs.s_name,
    rs.total_sales
  FROM RankedSales AS rs
  WHERE
    rs.sales_rank <= 10
)
SELECT
  p.p_partkey,
  p.p_name,
  ts.s_name,
  ts.total_sales,
  l.l_quantity,
  l.l_extendedprice,
  o.o_orderdate
FROM part AS p
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN TopSuppliers AS ts
  ON l.l_suppkey = ts.s_suppkey
WHERE
  o.o_orderdate >= CAST('1997-01-01' AS DATE)
  AND o.o_orderdate < CAST('1997-12-31' AS DATE)
ORDER BY
  ts.total_sales DESC,
  o.o_orderdate;
