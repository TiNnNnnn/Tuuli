WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    RANK() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS order_rank
  FROM orders AS o
), SupplierAggr AS (
  SELECT
    ps.ps_partkey,
    SUM(ps.ps_availqty) AS total_availqty,
    AVG(ps.ps_supplycost) AS avg_supplycost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
), CustomerOrderStats AS (
  SELECT
    c.c_custkey,
    COUNT(DISTINCT o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey
)
SELECT
  p.p_partkey,
  p.p_name,
  COALESCE(SA.total_availqty, 0) AS available_quantity,
  COALESCE(SA.avg_supplycost, 0) AS average_supply_cost,
  R.order_rank,
  COS(NULLIF(CUS.total_spent, 0)) AS adjusted_spending,
  CASE
    WHEN p.p_size > 20
    THEN 'Large'
    WHEN p.p_size IS NULL
    THEN 'Unknown'
    ELSE 'Small'
  END AS size_category
FROM part AS p
LEFT JOIN SupplierAggr AS SA
  ON p.p_partkey = SA.ps_partkey
LEFT JOIN RankedOrders AS R
  ON R.o_orderkey IN (
    SELECT
      o_orderkey
    FROM orders
    WHERE
      o_orderstatus = 'F'
      AND o_orderdate > (
        CAST('1998-10-01' AS DATE) - INTERVAL '365 DAYS'
      )
    FETCH FIRST 1 ROWS ONLY
  )
LEFT JOIN CustomerOrderStats AS CUS
  ON CUS.total_orders > 5
WHERE
  p.p_retailprice < ALL (
    SELECT
      p_retailprice
    FROM part
    WHERE
      p_brand = 'BrandA'
  )
  AND (
    p.p_comment LIKE '%fragile%' OR p.p_comment IS NULL
  )
ORDER BY
  available_quantity DESC,
  adjusted_spending ASC
FETCH FIRST 100 ROWS ONLY;
