WITH RECURSIVE SupplierHierarchy AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    0 AS level
  FROM supplier AS s
  WHERE
    s.s_nationkey IN (
      SELECT
        n.n_nationkey
      FROM nation AS n
      WHERE
        n.n_name = 'USA'
    )
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    sh.level + 1
  FROM supplier AS s
  JOIN SupplierHierarchy AS sh
    ON s.s_nationkey = sh.s_nationkey
  WHERE
    sh.level < 5
), AggregatedSales AS (
  SELECT
    c.c_custkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    COUNT(DISTINCT o.o_orderkey) AS order_count
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderdate >= '1997-01-01'
  GROUP BY
    c.c_custkey
), SupplierSales AS (
  SELECT
    sh.s_name AS supplier_name,
    SUM(l.l_extendedprice) AS supplier_total_sales
  FROM lineitem AS l
  JOIN partsupp AS ps
    ON l.l_partkey = ps.ps_partkey
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  JOIN SupplierHierarchy AS sh
    ON s.s_suppkey = sh.s_suppkey
  GROUP BY
    sh.s_name
), RegionSales AS (
  SELECT
    r.r_name AS region_name,
    SUM(ls.supplier_total_sales) AS total_region_sales
  FROM SupplierSales AS ls
  JOIN nation AS n
    ON n.n_nationkey = (
      SELECT DISTINCT
        s_nationkey
      FROM supplier AS s
      WHERE
        s.s_name = ls.supplier_name
    )
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  GROUP BY
    r.r_name
)
SELECT
  r.r_name AS region_name,
  COALESCE(ag.total_sales, 0) AS customer_total_sales,
  COALESCE(rg.total_region_sales, 0) AS region_sales
FROM region AS r
LEFT JOIN AggregatedSales AS ag
  ON ag.c_custkey = (
    SELECT
      c.c_custkey
    FROM customer AS c
    WHERE
      c.c_nationkey = (
        SELECT
          n.n_nationkey
        FROM nation AS n
        WHERE
          n.n_name = 'USA'
      )
    LIMIT 1
  )
LEFT JOIN RegionSales AS rg
  ON rg.region_name = r.r_name
ORDER BY
  r.r_name,
  customer_total_sales DESC;
