WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    c.c_name,
    c.c_acctbal,
    ROW_NUMBER() OVER (PARTITION BY c.c_nationkey ORDER BY o.o_orderdate DESC) AS OrderRank
  FROM orders AS o
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
), SupplierDetails AS (
  SELECT
    ps.ps_partkey,
    ps.ps_suppkey,
    s.s_name,
    SUM(ps.ps_availqty) AS TotalAvailableQty,
    AVG(ps.ps_supplycost) AS AvgSupplyCost
  FROM partsupp AS ps
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  GROUP BY
    ps.ps_partkey,
    ps.ps_suppkey,
    s.s_name
), HighValueCustOrders AS (
  SELECT
    ro.o_orderkey,
    ro.o_orderdate,
    ro.o_totalprice,
    sd.TotalAvailableQty,
    sd.AvgSupplyCost,
    ro.c_name
  FROM RankedOrders AS ro
  JOIN SupplierDetails AS sd
    ON ro.o_orderkey IN (
      SELECT
        l.l_orderkey
      FROM lineitem AS l
      WHERE
        l.l_partkey IN (
          SELECT
            ps.ps_partkey
          FROM partsupp AS ps
          WHERE
            ps.ps_suppkey = sd.ps_suppkey
        )
    )
  WHERE
    ro.OrderRank <= 5
    AND ro.o_totalprice > (
      SELECT
        AVG(o2.o_totalprice)
      FROM orders AS o2
    )
)
SELECT
  h.c_name,
  COUNT(h.o_orderkey) AS OrderCount,
  SUM(h.o_totalprice) AS TotalSpent,
  AVG(h.TotalAvailableQty) AS AvgQtyAvailable,
  AVG(h.AvgSupplyCost) AS AvgSupplyCost
FROM HighValueCustOrders AS h
GROUP BY
  h.c_name
ORDER BY
  TotalSpent DESC
LIMIT 10;
