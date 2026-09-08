WITH RECURSIVE SupplierHierarchy AS (
  SELECT
    s_suppkey,
    s_name AS s_suppliername,
    s_nationkey,
    s_acctbal,
    0 AS level
  FROM supplier
  WHERE
    s_acctbal > 10000
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    s.s_acctbal,
    sh.level + 1
  FROM supplier AS s
  JOIN SupplierHierarchy AS sh
    ON s.s_nationkey = sh.s_nationkey
  WHERE
    sh.level < 3
), RankedParts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_retailprice,
    ROW_NUMBER() OVER (PARTITION BY p.p_brand ORDER BY p.p_retailprice DESC) AS rank
  FROM part AS p
  WHERE
    NOT p.p_retailprice IS NULL AND p.p_size >= 10
), HighValueOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS order_value
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderstatus = 'F'
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
  HAVING
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) > 50000
), CustomerPreferential AS (
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_mktsegment,
    SUM(hv.order_value) AS total_order_value
  FROM customer AS c
  LEFT JOIN HighValueOrders AS hv
    ON c.c_custkey = hv.o_orderkey
  GROUP BY
    c.c_custkey,
    c.c_name,
    c.c_mktsegment
  HAVING
    NOT SUM(hv.order_value) IS NULL
), FinalReport AS (
  SELECT
    r.r_name,
    SUM(cp.total_order_value) AS regional_order_value
  FROM region AS r
  JOIN nation AS n
    ON r.r_regionkey = n.n_regionkey
  JOIN CustomerPreferential AS cp
    ON cp.c_mktsegment = n.n_name
  GROUP BY
    r.r_name
), SupplierSummary AS (
  SELECT
    sh.s_suppliername,
    SUM(sh.s_acctbal) AS total_balance,
    COUNT(DISTINCT cp.c_custkey) AS total_customers
  FROM SupplierHierarchy AS sh
  JOIN CustomerPreferential AS cp
    ON sh.s_nationkey = cp.c_custkey
  GROUP BY
    sh.s_suppliername
)
SELECT
  fs.r_name,
  fs.regional_order_value,
  ss.total_balance,
  ss.total_customers
FROM FinalReport AS fs
FULL OUTER JOIN SupplierSummary AS ss
  ON fs.r_name = ss.s_suppliername
WHERE
  NOT fs.regional_order_value IS NULL OR NOT ss.total_balance IS NULL
ORDER BY
  fs.regional_order_value DESC NULLS LAST,
  ss.total_balance DESC NULLS LAST;
