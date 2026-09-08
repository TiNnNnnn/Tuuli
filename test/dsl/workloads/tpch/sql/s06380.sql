WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    c.c_mktsegment,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_orderdate DESC) AS rn
  FROM orders AS o
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate < CAST('1997-10-01' AS DATE)
), SupplierStats AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
), PartStats AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_brand,
    COUNT(DISTINCT ps.ps_suppkey) AS total_suppliers,
    SUM(ps.ps_availqty) AS total_available
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  GROUP BY
    p.p_partkey,
    p.p_name,
    p.p_brand
)
SELECT
  o.o_orderkey,
  o.o_orderdate,
  o.o_totalprice,
  o.c_mktsegment,
  ps.p_name,
  ps.total_suppliers,
  ps.total_available,
  ss.total_supply_cost
FROM RankedOrders AS o
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN PartStats AS ps
  ON l.l_partkey = ps.p_partkey
JOIN SupplierStats AS ss
  ON l.l_suppkey = ss.s_suppkey
WHERE
  o.rn <= 100
ORDER BY
  o.o_orderdate DESC,
  ps.total_available DESC;
