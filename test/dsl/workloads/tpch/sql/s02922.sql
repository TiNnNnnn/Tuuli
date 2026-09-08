WITH SupplierStats AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_availqty) AS total_avail_qty,
    AVG(ps.ps_supplycost) AS avg_supply_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
), OrderDetails AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_order_value,
    COUNT(l.l_orderkey) AS total_items
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    l.l_shipdate >= '1997-01-01'
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
), HighValueOrders AS (
  SELECT
    od.o_orderkey,
    od.o_orderdate,
    od.total_order_value,
    od.total_items
  FROM OrderDetails AS od
  WHERE
    od.total_order_value > (
      SELECT
        AVG(total_order_value)
      FROM OrderDetails
    )
)
SELECT
  n.n_name AS nation,
  s.s_name AS supplier_name,
  ss.total_avail_qty,
  ss.avg_supply_cost,
  hvo.total_order_value,
  hvo.total_items
FROM nation AS n
LEFT JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
LEFT JOIN SupplierStats AS ss
  ON s.s_suppkey = ss.s_suppkey
LEFT JOIN lineitem AS l
  ON s.s_suppkey = l.l_suppkey
LEFT JOIN HighValueOrders AS hvo
  ON hvo.o_orderkey = l.l_orderkey
WHERE
  NOT ss.total_avail_qty IS NULL
  AND (
    hvo.total_order_value IS NULL OR hvo.total_order_value > 1000
  )
ORDER BY
  n.n_name,
  ss.avg_supply_cost DESC;
