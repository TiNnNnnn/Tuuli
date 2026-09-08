WITH SupplierAggregates AS (
  SELECT
    ps.ps_suppkey,
    SUM(ps.ps_availqty) AS total_available_quantity,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
  FROM partsupp AS ps
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  GROUP BY
    ps.ps_suppkey
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    COUNT(o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderstatus = 'O'
  GROUP BY
    c.c_custkey
), TopSuppliers AS (
  SELECT
    sa.ps_suppkey,
    sa.total_available_quantity,
    sa.total_supply_cost
  FROM SupplierAggregates AS sa
  WHERE
    sa.total_available_quantity > 1000
  ORDER BY
    sa.total_supply_cost DESC
  LIMIT 5
), HighestSpendingCustomers AS (
  SELECT
    co.c_custkey,
    co.total_orders,
    co.total_spent
  FROM CustomerOrders AS co
  WHERE
    co.total_spent > 5000
  ORDER BY
    co.total_spent DESC
  LIMIT 10
)
SELECT
  s.s_name AS supplier_name,
  s.s_phone AS supplier_phone,
  c.c_name AS customer_name,
  c.c_phone AS customer_phone,
  ts.total_available_quantity,
  ts.total_supply_cost,
  h.total_orders,
  h.total_spent
FROM TopSuppliers AS ts
JOIN supplier AS s
  ON ts.ps_suppkey = s.s_suppkey
JOIN HighestSpendingCustomers AS h
  ON h.total_spent > 5000
JOIN customer AS c
  ON h.c_custkey = c.c_custkey
ORDER BY
  ts.total_supply_cost DESC,
  h.total_spent DESC;
