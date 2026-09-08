WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    RANK() OVER (PARTITION BY n.n_nationkey ORDER BY s.s_acctbal DESC) AS rnk
  FROM supplier AS s
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
), HighValueOrders AS (
  SELECT
    o.o_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  GROUP BY
    o.o_orderkey
  HAVING
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) > 10000
), SupplierParts AS (
  SELECT
    ps.ps_partkey,
    ps.ps_suppkey,
    SUM(ps.ps_availqty) AS total_availqty
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey,
    ps.ps_suppkey
)
SELECT
  n.n_name,
  p.p_name,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  SUM(l.l_extendedprice) AS total_sales,
  COUNT(DISTINCT CASE WHEN r.rnk = 1 THEN s.s_suppkey END) AS top_supplier_count
FROM part AS p
LEFT JOIN SupplierParts AS sp
  ON sp.ps_partkey = p.p_partkey
LEFT JOIN supplier AS s
  ON s.s_suppkey = sp.ps_suppkey
LEFT JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
LEFT JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
LEFT JOIN RankedSuppliers AS r
  ON s.s_suppkey = r.s_suppkey
WHERE
  l.l_shipdate BETWEEN '1997-01-01' AND '1997-12-31'
  AND (
    l.l_returnflag = 'N' OR l.l_returnflag IS NULL
  )
  AND p.p_retailprice > 50
GROUP BY
  n.n_name,
  p.p_name
HAVING
  SUM(l.l_extendedprice) > (
    SELECT
      AVG(total_revenue)
    FROM HighValueOrders
  )
ORDER BY
  total_sales DESC,
  total_orders DESC;
