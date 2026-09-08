WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_custkey,
    o.o_totalprice,
    RANK() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_orderdate DESC) AS order_rank
  FROM orders AS o
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
), SupplierParts AS (
  SELECT
    ps.ps_partkey,
    SUM(ps.ps_availqty) AS total_available_quantity,
    AVG(ps.ps_supplycost) AS average_supply_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
), CustomerSegment AS (
  SELECT
    c.c_custkey,
    c.c_mktsegment,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_mktsegment
)
SELECT
  p.p_name,
  p.p_type,
  rp.o_orderkey,
  rp.o_totalprice,
  cs.total_spent,
  CASE WHEN cs.total_spent IS NULL THEN 'New Customer' ELSE 'Returning Customer' END AS customer_status,
  COALESCE(sp.total_available_quantity, 0) AS available_quantity,
  sp.average_supply_cost,
  RANK() OVER (ORDER BY COALESCE(sp.total_available_quantity, 0) DESC) AS supply_rank
FROM part AS p
LEFT JOIN SupplierParts AS sp
  ON p.p_partkey = sp.ps_partkey
LEFT JOIN RankedOrders AS rp
  ON rp.o_custkey = (
    SELECT
      c.c_custkey
    FROM customer AS c
    WHERE
      c.c_nationkey = (
        SELECT
          n.n_nationkey
        FROM nation AS n
        WHERE
          n.n_name = 'USA'
      )
  )
LEFT JOIN CustomerSegment AS cs
  ON cs.c_custkey = rp.o_custkey
WHERE
  p.p_retailprice > 100.00
  AND p.p_size IN (
    SELECT DISTINCT
      p2.p_size
    FROM part AS p2
    WHERE
      p2.p_type LIKE 'Medium%'
  )
ORDER BY
  supply_rank DESC,
  customer_status;
