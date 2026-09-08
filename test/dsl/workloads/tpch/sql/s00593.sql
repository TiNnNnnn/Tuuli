WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    RANK() OVER (PARTITION BY o.o_orderstatus ORDER BY SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) DESC) AS revenue_rank
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate < CAST('1997-12-31' AS DATE)
  GROUP BY
    o.o_orderkey,
    o.o_orderdate,
    o.o_orderstatus
), SupplierSummary AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    COUNT(ps.ps_partkey) AS parts_supplied,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
  HAVING
    COUNT(ps.ps_partkey) > 5
)
SELECT
  r.r_name,
  AVG(ss.total_supply_cost) AS avg_supply_cost,
  MAX(o.total_revenue) AS max_revenue
FROM region AS r
LEFT JOIN nation AS n
  ON r.r_regionkey = n.n_regionkey
LEFT JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
LEFT JOIN SupplierSummary AS ss
  ON s.s_suppkey = ss.s_suppkey
LEFT JOIN RankedOrders AS o
  ON o.o_orderkey = (
    SELECT
      MIN(o2.o_orderkey)
    FROM RankedOrders AS o2
    WHERE
      o2.o_orderdate >= CAST('1997-06-01' AS DATE)
  )
WHERE
  NOT r.r_name IS NULL AND NOT ss.parts_supplied IS NULL
GROUP BY
  r.r_name
HAVING
  AVG(ss.total_supply_cost) > (
    SELECT
      AVG(ps.ps_supplycost)
    FROM partsupp AS ps
  )
ORDER BY
  r.r_name ASC;
