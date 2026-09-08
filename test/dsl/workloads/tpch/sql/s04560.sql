WITH SupplierSales AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    COUNT(DISTINCT o.o_orderkey) AS order_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN lineitem AS l
    ON ps.ps_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  WHERE
    o.o_orderdate >= '1997-01-01' AND o.o_orderdate <= '1997-12-31'
  GROUP BY
    s.s_suppkey,
    s.s_name
), TopSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    RANK() OVER (ORDER BY ss.total_sales DESC) AS rank
  FROM supplier AS s
  JOIN SupplierSales AS ss
    ON s.s_suppkey = ss.s_suppkey
  WHERE
    ss.total_sales > 0
)
SELECT
  n.n_name AS nation_name,
  SUM(ss.total_sales) AS nation_sales,
  COUNT(DISTINCT ts.s_suppkey) AS supplier_count
FROM nation AS n
LEFT JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
LEFT JOIN SupplierSales AS ss
  ON s.s_suppkey = ss.s_suppkey
LEFT JOIN TopSuppliers AS ts
  ON ts.s_suppkey = s.s_suppkey
WHERE
  NOT n.n_name IS NULL
GROUP BY
  n.n_name
HAVING
  SUM(ss.total_sales) > (
    SELECT
      AVG(total_sales)
    FROM SupplierSales
  )
  OR SUM(ss.total_sales) IS NULL
ORDER BY
  nation_sales DESC;
