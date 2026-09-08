WITH RECURSIVE CustomerOrderTotals AS (
  SELECT
    c.c_custkey,
    c.c_name,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
  GROUP BY
    c.c_custkey,
    c.c_name
), RankedCustomers AS (
  SELECT
    c.*,
    RANK() OVER (ORDER BY total_spent DESC) AS rank
  FROM CustomerOrderTotals AS c
  WHERE
    total_spent > 1000
), SupplierAvgPrices AS (
  SELECT
    ps.ps_suppkey,
    AVG(ps.ps_supplycost) AS avg_supply_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_suppkey
), HighValueParts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_retailprice,
    COALESCE(ps.ps_availqty, 0) AS available_quantity
  FROM part AS p
  LEFT JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  WHERE
    p.p_retailprice > 50.00
)
SELECT
  rc.c_name,
  rc.total_spent,
  spp.avg_supply_cost,
  hpp.p_name,
  hpp.available_quantity
FROM RankedCustomers AS rc
JOIN SupplierAvgPrices AS spp
  ON rc.rank <= 10
JOIN HighValueParts AS hpp
  ON spp.ps_suppkey IN (
    SELECT
      ps.ps_suppkey
    FROM partsupp AS ps
    JOIN lineitem AS l
      ON ps.ps_partkey = l.l_partkey
    WHERE
      l.l_quantity > 50
  )
ORDER BY
  rc.total_spent DESC,
  spp.avg_supply_cost ASC;
