WITH NationwideSales AS (
  SELECT
    n.n_name AS nation,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM lineitem AS l
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
  JOIN nation AS n
    ON c.c_nationkey = n.n_nationkey
  WHERE
    o.o_orderdate >= CAST('1996-01-01' AS DATE)
    AND o.o_orderdate < CAST('1997-01-01' AS DATE)
  GROUP BY
    n.n_name
), SalesBySupplier AS (
  SELECT
    s.s_name AS supplier_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS supplier_sales
  FROM lineitem AS l
  JOIN partsupp AS ps
    ON l.l_partkey = ps.ps_partkey
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  GROUP BY
    s.s_name
), TopSellingParts AS (
  SELECT
    p.p_name AS part_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS part_sales
  FROM lineitem AS l
  JOIN partsupp AS ps
    ON l.l_partkey = ps.ps_partkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  GROUP BY
    p.p_name
  ORDER BY
    part_sales DESC
  LIMIT 10
)
SELECT
  ns.nation,
  ns.total_sales,
  ss.supplier_name,
  ss.supplier_sales,
  tp.part_name,
  tp.part_sales
FROM NationwideSales AS ns
JOIN SalesBySupplier AS ss
  ON ss.supplier_sales > 100000
JOIN TopSellingParts AS tp
  ON tp.part_sales > 50000
ORDER BY
  ns.total_sales DESC,
  ss.supplier_sales DESC,
  tp.part_sales DESC
LIMIT 50;
