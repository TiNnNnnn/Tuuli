WITH RegionSupplier AS (
  SELECT
    r.r_name AS region_name,
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_availqty) AS total_available_qty
  FROM region AS r
  JOIN nation AS n
    ON r.r_regionkey = n.n_regionkey
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    r.r_name,
    s.s_suppkey,
    s.s_name
), TopSuppliers AS (
  SELECT
    region_name,
    s_suppkey,
    s_name,
    total_available_qty,
    RANK() OVER (PARTITION BY region_name ORDER BY total_available_qty DESC) AS rank
  FROM RegionSupplier
)
SELECT
  ts.region_name,
  COUNT(DISTINCT c.c_custkey) AS customer_count,
  SUM(o.o_totalprice) AS total_revenue,
  AVG(o.o_totalprice) AS avg_order_value,
  STRING_AGG(ts.s_name, ', ') AS top_suppliers
FROM TopSuppliers AS ts
JOIN lineitem AS l
  ON l.l_suppkey = ts.s_suppkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  ts.rank <= 5
GROUP BY
  ts.region_name
ORDER BY
  total_revenue DESC;
