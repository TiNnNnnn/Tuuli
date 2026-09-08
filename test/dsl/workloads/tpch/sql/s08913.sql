WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS TotalSupplyCost,
    RANK() OVER (ORDER BY SUM(ps.ps_supplycost * ps.ps_availqty) DESC) AS SupplyRank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_acctbal
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_mktsegment,
    SUM(o.o_totalprice) AS TotalOrderValue
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderstatus = 'F'
  GROUP BY
    c.c_custkey,
    c.c_name,
    c.c_mktsegment
), PartSales AS (
  SELECT
    l.l_partkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS TotalSales
  FROM lineitem AS l
  WHERE
    l.l_shipdate >= CAST('1997-01-01' AS DATE)
    AND l.l_shipdate < CAST('1998-01-01' AS DATE)
  GROUP BY
    l.l_partkey
)
SELECT
  ps.p_name,
  ps.p_brand,
  rs.TotalSupplyCost,
  co.TotalOrderValue,
  psales.TotalSales
FROM part AS ps
JOIN RankedSuppliers AS rs
  ON ps.p_partkey = (
    SELECT
      ps_partkey
    FROM partsupp
    WHERE
      ps_suppkey = rs.s_suppkey
    LIMIT 1
  )
JOIN PartSales AS psales
  ON ps.p_partkey = psales.l_partkey
JOIN CustomerOrders AS co
  ON co.TotalOrderValue > 10000
WHERE
  rs.SupplyRank <= 5
ORDER BY
  rs.TotalSupplyCost DESC,
  co.TotalOrderValue DESC;
