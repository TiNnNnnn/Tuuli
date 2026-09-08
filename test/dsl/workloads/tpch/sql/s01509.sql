WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY s.s_acctbal DESC) AS rn
  FROM supplier AS s
  WHERE
    s.s_acctbal > (
      SELECT
        AVG(s2.s_acctbal)
      FROM supplier AS s2
    )
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    COUNT(o.o_orderkey) AS order_count,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey
), HighValueCustomers AS (
  SELECT
    cust.c_custkey,
    cust.order_count,
    cust.total_spent,
    RANK() OVER (ORDER BY cust.total_spent DESC) AS rank
  FROM CustomerOrders AS cust
  WHERE
    cust.total_spent > 1000
)
SELECT
  n.n_name AS nation_name,
  COALESCE(SUM(l.l_extendedprice * (
    1 - l.l_discount
  )), 0) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  MAX(s.s_name) AS top_supplier
FROM nation AS n
LEFT JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
LEFT JOIN partsupp AS ps
  ON s.s_suppkey = ps.ps_suppkey
LEFT JOIN part AS p
  ON ps.ps_partkey = p.p_partkey
LEFT JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
LEFT JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  n.n_nationkey IN (
    SELECT DISTINCT
      s_nationkey
    FROM RankedSuppliers
    WHERE
      rn <= 3
  )
GROUP BY
  n.n_name
UNION ALL
SELECT
  'High Value Customers' AS nation_name,
  SUM(COALESCE(o.o_totalprice, 0)) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  NULL AS top_supplier
FROM HighValueCustomers AS hvc
LEFT JOIN orders AS o
  ON hvc.c_custkey = o.o_custkey
WHERE
  hvc.rank <= 10
GROUP BY
  hvc.c_custkey,
  hvc.order_count,
  hvc.total_spent
ORDER BY
  total_revenue DESC;
