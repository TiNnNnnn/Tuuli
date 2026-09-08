WITH RegionalSales AS (
  SELECT
    r.r_name AS region_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM region AS r
  JOIN nation AS n
    ON r.r_regionkey = n.n_regionkey
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate < CAST('1998-01-01' AS DATE)
  GROUP BY
    r.r_name
), TopRegions AS (
  SELECT
    region_name,
    total_sales,
    DENSE_RANK() OVER (ORDER BY total_sales DESC) AS sales_rank
  FROM RegionalSales
)
SELECT
  region_name,
  total_sales
FROM TopRegions
WHERE
  sales_rank <= 5
ORDER BY
  total_sales DESC;
