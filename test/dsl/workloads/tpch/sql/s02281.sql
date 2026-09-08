WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY s.s_acctbal DESC) AS rn
  FROM supplier AS s
), AggregatedOrders AS (
  SELECT
    o.o_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    o.o_orderdate
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    l.l_shipdate >= CAST('1997-01-01' AS DATE)
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
), TotalRevenueByNation AS (
  SELECT
    n.n_name,
    SUM(a.total_revenue) AS total_revenue
  FROM nation AS n
  JOIN customer AS c
    ON n.n_nationkey = c.c_nationkey
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  JOIN AggregatedOrders AS a
    ON o.o_orderkey = a.o_orderkey
  GROUP BY
    n.n_name
), HighestRevenueRegion AS (
  SELECT
    r.r_name,
    SUM(t.total_revenue) AS region_revenue
  FROM region AS r
  JOIN nation AS n
    ON r.r_regionkey = n.n_regionkey
  JOIN TotalRevenueByNation AS t
    ON n.n_name = t.n_name
  GROUP BY
    r.r_name
)
SELECT
  r.r_name,
  COALESCE(h.region_revenue, 0) AS total_revenue,
  COUNT(DISTINCT s.s_suppkey) AS supplier_count
FROM region AS r
LEFT JOIN HighestRevenueRegion AS h
  ON r.r_name = h.r_name
LEFT JOIN RankedSuppliers AS s
  ON s.rn <= 5
GROUP BY
  r.r_name,
  h.region_revenue
ORDER BY
  total_revenue DESC,
  supplier_count DESC;
