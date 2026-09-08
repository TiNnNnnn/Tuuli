WITH SupplierStats AS (
  SELECT
    s_name,
    COUNT(DISTINCT ps_partkey) AS total_parts,
    SUM(ps_supplycost * ps_availqty) AS total_value
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s_name
), OrderSummary AS (
  SELECT
    o.o_custkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    COUNT(DISTINCT o.o_orderkey) AS order_count
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
  GROUP BY
    o.o_custkey
)
SELECT
  c.c_name,
  c.c_acctbal,
  c.c_mktsegment,
  ss.total_parts,
  ss.total_value,
  os.total_revenue,
  os.order_count
FROM customer AS c
LEFT JOIN SupplierStats AS ss
  ON ss.total_value > 100000
LEFT JOIN OrderSummary AS os
  ON c.c_custkey = os.o_custkey
WHERE
  c.c_acctbal > 5000
ORDER BY
  total_revenue DESC NULLS LAST,
  c.c_name ASC
LIMIT 100;
