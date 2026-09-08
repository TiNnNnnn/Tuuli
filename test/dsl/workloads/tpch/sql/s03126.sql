WITH CustomerOrders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    COUNT(o.o_orderkey) AS order_count,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name
), HighValueSuppliers AS (
  SELECT
    ps.ps_suppkey,
    SUM(ps.ps_supplycost) AS total_supply_cost
  FROM partsupp AS ps
  WHERE
    ps.ps_availqty > 100
  GROUP BY
    ps.ps_suppkey
  HAVING
    SUM(ps.ps_supplycost) > 50000
), NationRegion AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    r.r_name AS region_name
  FROM nation AS n
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
)
SELECT
  c.c_name,
  co.order_count,
  co.total_spent,
  nr.region_name,
  hs.ps_suppkey AS high_value_supplier,
  COALESCE(ps.ps_availqty, 0) AS available_quantity
FROM CustomerOrders AS co
JOIN customer AS c
  ON co.c_custkey = c.c_custkey
LEFT JOIN NationRegion AS nr
  ON c.c_nationkey = nr.n_nationkey
LEFT JOIN HighValueSuppliers AS hs
  ON hs.ps_suppkey = (
    SELECT
      ps.ps_suppkey
    FROM partsupp AS ps
    JOIN lineitem AS l
      ON ps.ps_partkey = l.l_partkey
    WHERE
      l.l_orderkey IN (
        SELECT
          o.o_orderkey
        FROM orders AS o
        WHERE
          o.o_custkey = c.c_custkey
      )
    GROUP BY
      ps.ps_suppkey
    ORDER BY
      SUM(l.l_extendedprice) DESC
    LIMIT 1
  )
LEFT JOIN partsupp AS ps
  ON ps.ps_suppkey = hs.ps_suppkey
WHERE
  co.total_spent > 1000
ORDER BY
  co.total_spent DESC,
  co.order_count ASC;
