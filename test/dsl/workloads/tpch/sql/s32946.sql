WITH RECURSIVE OrderHierarchy AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderkey ORDER BY o.o_orderdate) AS rn
  FROM orders AS o
  WHERE
    o.o_orderdate >= '1997-01-01'
  UNION ALL
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderkey ORDER BY o.o_orderdate) AS rn
  FROM orders AS o
  INNER JOIN OrderHierarchy AS oh
    ON o.o_orderkey = oh.o_orderkey
  WHERE
    oh.rn < 5
), AvgSupplierCost AS (
  SELECT
    ps.ps_partkey,
    AVG(ps.ps_supplycost) AS avg_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
), PartDetails AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_brand,
    p.p_retailprice,
    COALESCE(ps.avg_cost, 0) AS avg_supply_cost,
    (
      p.p_retailprice - COALESCE(ps.avg_cost, 0)
    ) AS price_margin
  FROM part AS p
  LEFT JOIN AvgSupplierCost AS ps
    ON p.p_partkey = ps.ps_partkey
)
SELECT
  d.p_name,
  d.p_brand,
  d.p_retailprice,
  d.avg_supply_cost,
  d.price_margin,
  COUNT(DISTINCT l.l_orderkey) AS order_count,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  STRING_AGG(DISTINCT c.c_name, ', ') AS customer_names
FROM PartDetails AS d
LEFT JOIN lineitem AS l
  ON d.p_partkey = l.l_partkey
LEFT JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
LEFT JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  d.price_margin > 10.00 AND o.o_orderstatus = 'F'
GROUP BY
  d.p_partkey,
  d.p_name,
  d.p_brand,
  d.p_retailprice,
  d.avg_supply_cost,
  d.price_margin
ORDER BY
  total_revenue DESC
LIMIT 10;
