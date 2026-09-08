WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    o.o_orderstatus,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_orderdate DESC) AS rn
  FROM orders AS o
  WHERE
    o.o_orderdate >= CURRENT_DATE - INTERVAL '1 YEAR'
), PartSupplierDetails AS (
  SELECT
    ps.ps_partkey,
    ps.ps_suppkey,
    SUM(ps.ps_availqty) AS total_avail_qty,
    AVG(ps.ps_supplycost) AS avg_supply_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey,
    ps.ps_suppkey
), CustomerOrderStats AS (
  SELECT
    c.c_custkey,
    COUNT(DISTINCT o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    c.c_acctbal > 0
  GROUP BY
    c.c_custkey
)
SELECT
  p.p_partkey,
  p.p_name,
  p.p_brand,
  p.p_retailprice,
  s.s_name AS supplier_name,
  psd.total_avail_qty,
  psd.avg_supply_cost,
  co.total_orders,
  co.total_spent,
  CASE
    WHEN co.total_orders IS NULL
    THEN 'No Orders'
    WHEN co.total_spent >= 1000
    THEN 'Premium Customer'
    ELSE 'Regular Customer'
  END AS customer_category,
  RANK() OVER (ORDER BY p.p_retailprice DESC) AS price_rank
FROM part AS p
LEFT JOIN PartSupplierDetails AS psd
  ON p.p_partkey = psd.ps_partkey
LEFT JOIN supplier AS s
  ON psd.ps_suppkey = s.s_suppkey
LEFT JOIN CustomerOrderStats AS co
  ON s.s_nationkey = co.c_custkey
WHERE
  p.p_size BETWEEN 10 AND 20
  AND (
    psd.total_avail_qty IS NULL OR psd.total_avail_qty > 100
  )
ORDER BY
  price_rank,
  p.p_name;
