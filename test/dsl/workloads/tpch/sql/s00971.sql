WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    RANK() OVER (PARTITION BY ps_partkey ORDER BY s.s_acctbal DESC) AS rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
), CustomerOrderStats AS (
  SELECT
    c.c_custkey,
    COUNT(DISTINCT o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent,
    AVG(o.o_totalprice) AS avg_order_value
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey
), LineItemStats AS (
  SELECT
    l.l_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS net_line_value,
    SUM(l.l_quantity) AS total_quantity,
    COUNT(DISTINCT l.l_partkey) AS distinct_parts
  FROM lineitem AS l
  GROUP BY
    l.l_orderkey
)
SELECT
  c.c_name,
  cs.total_orders,
  cs.total_spent,
  cs.avg_order_value,
  COALESCE(rs.s_name, 'No Supplier') AS top_supplier,
  COALESCE(rs.s_acctbal, 0) AS top_supplier_acctbal,
  lis.total_quantity,
  lis.net_line_value
FROM CustomerOrderStats AS cs
JOIN customer AS c
  ON cs.c_custkey = c.c_custkey
LEFT JOIN RankedSuppliers AS rs
  ON cs.c_custkey IN (
    SELECT DISTINCT
      o.o_custkey
    FROM orders AS o
    WHERE
      o.o_orderkey IN (
        SELECT
          l.l_orderkey
        FROM lineitem AS l
        WHERE
          l.l_shipdate >= CAST('1997-01-01' AS DATE)
      )
  )
LEFT JOIN LineItemStats AS lis
  ON cs.c_custkey IN (
    SELECT DISTINCT
      o.o_custkey
    FROM orders AS o
    WHERE
      o.o_orderkey = lis.l_orderkey
  )
WHERE
  cs.total_spent > 1000 AND cs.avg_order_value BETWEEN 200 AND 500
ORDER BY
  cs.total_spent DESC;
