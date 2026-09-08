WITH RECURSIVE supplier_costs AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
), customer_orders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    o.o_orderkey,
    o.o_orderstatus,
    o.o_totalprice,
    CASE WHEN o.o_orderstatus = 'F' THEN 'Completed' ELSE 'Pending' END AS order_status_desc
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderdate >= '1997-01-01'
), line_item_summary AS (
  SELECT
    l.l_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS net_revenue,
    COUNT(l.l_linenumber) AS total_items
  FROM lineitem AS l
  GROUP BY
    l.l_orderkey
), nations AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    r.r_name,
    COUNT(s.s_suppkey) AS supplier_count
  FROM nation AS n
  LEFT JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  LEFT JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  GROUP BY
    n.n_nationkey,
    n.n_name,
    r.r_name
)
SELECT
  ns.n_name,
  ns.r_name,
  ns.supplier_count,
  c.c_name,
  COALESCE(so.total_cost, 0) AS supplier_total_cost,
  COALESCE(lis.net_revenue, 0) AS order_net_revenue,
  CASE
    WHEN ns.supplier_count > 0
    THEN 'Active Supplier'
    WHEN ns.supplier_count = 0 AND NOT so.total_cost IS NULL
    THEN 'Inactive Supplier'
    ELSE 'Unknown Supplier Status'
  END AS supplier_status
FROM nations AS ns
LEFT JOIN supplier_costs AS so
  ON ns.n_nationkey = so.s_suppkey
LEFT JOIN customer_orders AS c
  ON ns.n_nationkey = c.c_custkey
LEFT JOIN line_item_summary AS lis
  ON c.o_orderkey = lis.l_orderkey
WHERE
  (
    NOT so.total_cost IS NULL OR NOT lis.net_revenue IS NULL
  )
  AND (
    NOT c.o_orderkey IS NULL OR ns.supplier_count > 0
  )
ORDER BY
  ns.n_name,
  supplier_total_cost DESC,
  order_net_revenue DESC
LIMIT 100
OFFSET (
  SELECT
    COUNT(*)
  FROM customer_orders
  WHERE
    order_status_desc = 'Completed'
) % 10;
