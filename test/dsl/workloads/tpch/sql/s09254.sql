WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    o.o_orderstatus,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_orderdate DESC) AS order_rank
  FROM orders AS o
  WHERE
    o.o_orderdate >= CURRENT_DATE - INTERVAL '6 MONTHS'
), SupplierParts AS (
  SELECT
    ps.ps_partkey,
    ps.ps_suppkey,
    SUM(ps.ps_availqty) AS total_available_quantity,
    SUM(ps.ps_supplycost) AS total_supply_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey,
    ps.ps_suppkey
), CustomerGroup AS (
  SELECT
    c.c_custkey,
    c.c_mktsegment,
    COUNT(o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_mktsegment
)
SELECT
  rg.n_name AS nation_name,
  COUNT(DISTINCT r.r_regionkey) AS region_count,
  SUM(cg.total_orders) AS total_orders_from_customers,
  AVG(cg.total_spent) AS average_spent_per_customer,
  AVG(sp.total_available_quantity) AS average_available_quantity,
  AVG(sp.total_supply_cost) AS average_supply_cost
FROM nation AS rg
JOIN region AS r
  ON rg.n_regionkey = r.r_regionkey
LEFT JOIN CustomerGroup AS cg
  ON rg.n_nationkey = cg.c_custkey
LEFT JOIN SupplierParts AS sp
  ON sp.ps_suppkey IN (
    SELECT
      s.s_suppkey
    FROM supplier AS s
    WHERE
      s.s_nationkey = rg.n_nationkey
  )
GROUP BY
  rg.n_name,
  rg.n_nationkey
ORDER BY
  total_orders_from_customers DESC
LIMIT 100;
