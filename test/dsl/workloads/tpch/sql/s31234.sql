WITH RECURSIVE nation_hierarchy AS (
  SELECT
    n_nationkey,
    n_name,
    n_regionkey,
    0 AS level
  FROM nation
  WHERE
    n_regionkey = (
      SELECT
        r_regionkey
      FROM region
      WHERE
        r_name = 'ASIA'
    )
  UNION ALL
  SELECT
    n.n_nationkey,
    n.n_name,
    n.n_regionkey,
    nh.level + 1
  FROM nation AS n
  JOIN nation_hierarchy AS nh
    ON n.n_regionkey = nh.n_nationkey
), ranked_orders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS order_rank
  FROM orders AS o
  WHERE
    o.o_orderdate >= CAST('1996-01-01' AS DATE)
), supplier_summary AS (
  SELECT
    s.s_nationkey,
    SUM(ps.ps_availqty) AS total_avail_qty,
    SUM(ps.ps_supplycost) AS total_supply_cost,
    COUNT(DISTINCT s.s_suppkey) AS supplier_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_nationkey
)
SELECT
  n.n_name,
  SUM(lo.l_extendedprice * (
    1 - lo.l_discount
  )) AS total_revenue,
  SUM(CASE WHEN lo.l_returnflag = 'R' THEN lo.l_quantity ELSE 0 END) AS total_returned_qty,
  s.total_avail_qty,
  s.total_supply_cost,
  s.supplier_count,
  o.order_rank
FROM lineitem AS lo
JOIN ranked_orders AS o
  ON lo.l_orderkey = o.o_orderkey
JOIN supplier_summary AS s
  ON s.s_nationkey = (
    SELECT
      n_nationkey
    FROM nation
    WHERE
      n_name = 'GERMANY'
  )
JOIN nation_hierarchy AS n
  ON n.n_nationkey = s.s_nationkey
WHERE
  lo.l_shipdate BETWEEN CAST('1996-01-01' AS DATE) AND CAST('1996-12-31' AS DATE)
GROUP BY
  n.n_name,
  s.total_avail_qty,
  s.total_supply_cost,
  s.supplier_count,
  o.order_rank
HAVING
  SUM(lo.l_extendedprice * (
    1 - lo.l_discount
  )) > 10000
ORDER BY
  total_revenue DESC NULLS LAST;
