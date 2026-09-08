WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS rn
  FROM orders AS o
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate <= CAST('1997-12-31' AS DATE)
), SupplierParts AS (
  SELECT
    ps.ps_partkey,
    ps.ps_suppkey,
    SUM(ps.ps_availqty) AS total_available,
    AVG(ps.ps_supplycost) AS avg_supplycost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey,
    ps.ps_suppkey
), TopSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    COUNT(DISTINCT l.l_orderkey) AS order_count
  FROM supplier AS s
  JOIN SupplierParts AS sp
    ON s.s_suppkey = sp.ps_suppkey
  JOIN lineitem AS l
    ON l.l_partkey = sp.ps_partkey
  WHERE
    NOT l.l_shipdate IS NULL
  GROUP BY
    s.s_suppkey,
    s.s_name
  HAVING
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) > 100000
), CustomerInfo AS (
  SELECT
    c.c_custkey,
    c.c_name,
    COALESCE(SUM(o.o_totalprice), 0) AS total_spent,
    ROW_NUMBER() OVER (ORDER BY COALESCE(SUM(o.o_totalprice), 0) DESC) AS cust_rn
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name
  HAVING
    COUNT(o.o_orderkey) > 0
)
SELECT
  r.r_name,
  COALESCE(c.c_name, 'Unknown Customer') AS customer_name,
  COALESCE(c.total_spent, 0) AS total_spent,
  t.total_revenue,
  t.order_count,
  CASE
    WHEN NOT t.total_revenue IS NULL
    THEN (
      t.total_revenue / NULLIF(c.total_spent, 0)
    ) * 100
    ELSE 0
  END AS revenue_ratio,
  DENSE_RANK() OVER (PARTITION BY r.r_name ORDER BY t.total_revenue DESC) AS revenue_rank
FROM nation AS n
LEFT JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
LEFT JOIN CustomerInfo AS c
  ON n.n_nationkey = c.cust_rn
LEFT JOIN TopSuppliers AS t
  ON n.n_nationkey = t.s_suppkey
WHERE
  (
    c.total_spent > 5000 OR t.total_revenue > 20000
  )
  AND (
    t.order_count > 10 OR c.cust_rn IS NULL
  )
ORDER BY
  r.r_name,
  revenue_rank;
