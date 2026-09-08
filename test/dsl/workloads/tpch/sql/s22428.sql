WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY s.s_acctbal DESC) AS rank
  FROM supplier AS s
), HighValueCustomers AS (
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_acctbal,
    COUNT(o.o_orderkey) AS order_count
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    c.c_acctbal > (
      SELECT
        AVG(c2.c_acctbal)
      FROM customer AS c2
      WHERE
        NOT c2.c_acctbal IS NULL
    )
  GROUP BY
    c.c_custkey,
    c.c_name,
    c.c_acctbal
), SupplierStats AS (
  SELECT
    ps.ps_partkey,
    COUNT(ps.ps_suppkey) AS supplier_count,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_value
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
)
SELECT
  p.p_partkey,
  p.p_name,
  COALESCE(rn.s_name, 'No Suppliers') AS supplier_name,
  COALESCE(hc.c_name, 'No High Value Customers') AS customer_name,
  CASE
    WHEN ss.supplier_count IS NULL
    THEN 'No Supply'
    ELSE CAST(ss.supplier_count AS VARCHAR) || ' Suppliers'
  END AS supplier_description,
  CASE WHEN hc.order_count > 0 THEN 'Frequent Buyer' ELSE 'One-time Buyer' END AS buyer_status,
  ROW_NUMBER() OVER (PARTITION BY p.p_type ORDER BY p.p_retailprice DESC) AS part_rank
FROM part AS p
LEFT JOIN RankedSuppliers AS rn
  ON rn.rank = 1
LEFT JOIN HighValueCustomers AS hc
  ON hc.c_custkey = (
    SELECT
      c.c_custkey
    FROM customer AS c
    WHERE
      c.c_acctbal = (
        SELECT
          MAX(c2.c_acctbal)
        FROM customer AS c2
      )
  )
LEFT JOIN SupplierStats AS ss
  ON ss.ps_partkey = p.p_partkey
WHERE
  p.p_size BETWEEN 1 AND 50
  AND (
    NOT p.p_retailprice IS NULL OR p.p_comment LIKE '%Special%'
  )
ORDER BY
  p.p_partkey DESC
FETCH FIRST 10 ROWS ONLY;
