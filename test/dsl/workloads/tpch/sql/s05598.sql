WITH NationSummary AS (
  SELECT
    n.n_name AS nation_name,
    COUNT(DISTINCT s.s_suppkey) AS supplier_count,
    SUM(s.s_acctbal) AS total_acctbal,
    AVG(s.s_acctbal) AS avg_acctbal
  FROM nation AS n
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  GROUP BY
    n.n_name
), OrderSummary AS (
  SELECT
    c.c_nationkey,
    COUNT(DISTINCT o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_order_value,
    AVG(o.o_totalprice) AS avg_order_value,
    SUM(CASE WHEN o.o_orderstatus = 'F' THEN 1 ELSE 0 END) AS completed_orders
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_nationkey
)
SELECT
  ns.nation_name,
  ns.supplier_count,
  ns.total_acctbal,
  ns.avg_acctbal,
  os.total_orders,
  os.total_order_value,
  os.avg_order_value,
  os.completed_orders
FROM NationSummary AS ns
LEFT JOIN OrderSummary AS os
  ON ns.nation_name = (
    SELECT
      n.n_name
    FROM nation AS n
    WHERE
      n.n_nationkey = os.c_nationkey
  )
ORDER BY
  ns.supplier_count DESC,
  os.total_order_value DESC;
