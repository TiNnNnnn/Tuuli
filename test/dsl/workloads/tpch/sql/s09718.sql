WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    SUM(l.l_quantity) AS total_quantity,
    COUNT(DISTINCT l.l_partkey) AS distinct_parts,
    RANK() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS order_rank
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    o.o_orderdate >= CAST('1990-01-01' AS DATE)
    AND o.o_orderdate < CAST('1995-01-01' AS DATE)
  GROUP BY
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    o.o_orderstatus
), TopSuppliers AS (
  SELECT
    ps.ps_suppkey,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
  FROM partsupp AS ps
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  WHERE
    s.s_acctbal > 10000
  GROUP BY
    ps.ps_suppkey
  ORDER BY
    total_supply_cost DESC
  LIMIT 5
)
SELECT
  r.o_orderkey,
  r.o_orderdate,
  r.o_totalprice,
  r.total_quantity,
  r.distinct_parts,
  ts.total_supply_cost
FROM RankedOrders AS r
LEFT JOIN TopSuppliers AS ts
  ON r.o_orderkey = ts.ps_suppkey
WHERE
  r.order_rank <= 10
ORDER BY
  r.o_totalprice DESC,
  r.o_orderdate;
