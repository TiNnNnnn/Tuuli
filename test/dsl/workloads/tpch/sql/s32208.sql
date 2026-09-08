WITH RECURSIVE SupplierHierarchy AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    0 AS level
  FROM supplier AS s
  WHERE
    s.s_acctbal > 10000
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
    sh.level < 3
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderstatus = 'O'
    AND o.o_totalprice > (
      SELECT
        AVG(o2.o_totalprice)
      FROM orders AS o2
      WHERE
        o2.o_orderdate > CAST('1996-01-01' AS DATE)
    )
), PartSuppliers AS (
  SELECT
    p.p_partkey,
    p.p_name,
    SUM(ps.ps_availqty) AS total_available_qty,
    COUNT(DISTINCT ps.ps_suppkey) AS supplier_count
  FROM part AS p
  LEFT JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  GROUP BY
    p.p_partkey,
    p.p_name
)
SELECT
  co.c_custkey,
  co.c_name,
  ps.p_name,
  ps.total_available_qty,
  co.o_orderkey,
  co.o_orderdate,
  co.o_totalprice,
  ROW_NUMBER() OVER (PARTITION BY co.c_custkey ORDER BY co.o_totalprice DESC) AS order_rank,
  CASE WHEN ps.total_available_qty IS NULL THEN 'Out Of Stock' ELSE 'In Stock' END AS stock_status,
  REPLACE(co.c_name, 'Customer', 'Client') AS modified_customer_name
FROM CustomerOrders AS co
JOIN PartSuppliers AS ps
  ON ps.p_partkey = (
    SELECT
      l.l_partkey
    FROM lineitem AS l
    WHERE
      l.l_orderkey = co.o_orderkey
    LIMIT 1
  )
JOIN SupplierHierarchy AS sh
  ON sh.s_nationkey = COALESCE(
    (
      SELECT DISTINCT
        sh2.s_nationkey
      FROM SupplierHierarchy AS sh2
      WHERE
        sh2.level = 0
      LIMIT 1
    ),
    0
  )
WHERE
  co.o_orderdate >= (
    CAST('1998-10-01' AS DATE) - INTERVAL '1 YEAR'
  )
ORDER BY
  stock_status,
  order_rank DESC;
