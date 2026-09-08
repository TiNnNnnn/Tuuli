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
), TopSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    ss.total_sales,
    RANK() OVER (ORDER BY ss.total_sales DESC) AS sales_rank
  FROM supplier AS s
  JOIN SupplierSales AS ss
    ON s.s_suppkey = ss.s_suppkey
)
SELECT
  ts.s_suppkey,
  ts.s_name,
  ts.total_sales,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  AVG(o.o_totalprice) AS avg_order_value
FROM TopSuppliers AS ts
LEFT JOIN partsupp AS ps
  ON ts.s_suppkey = ps.ps_suppkey
LEFT JOIN lineitem AS l
  ON ps.ps_partkey = l.l_partkey
LEFT JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  ts.sales_rank <= 5
GROUP BY
  ts.s_suppkey,
  ts.s_name,
  ts.total_sales
ORDER BY
  ts.total_sales DESC;
