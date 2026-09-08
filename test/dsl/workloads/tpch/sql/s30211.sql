WITH RECURSIVE top_suppliers AS (
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
  HAVING
    SUM(ps.ps_supplycost * ps.ps_availqty) > 10000
), dates AS (
  SELECT
    o.o_orderdate,
    DENSE_RANK() OVER (ORDER BY o.o_orderdate) AS order_rank
  FROM orders AS o
  GROUP BY
    o.o_orderdate
), lineitem_summary AS (
  SELECT
    l.l_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    MAX(l.l_shipdate) AS last_shipdate
  FROM lineitem AS l
  WHERE
    l.l_shipdate >= '1997-01-01' AND l.l_shipdate <= '1997-12-31'
  GROUP BY
    l.l_orderkey
)
SELECT
  n.n_name,
  COALESCE(SUM(ls.total_revenue), 0) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  MAX(s.s_name) AS top_supplier,
  r.r_name
FROM nation AS n
LEFT JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
LEFT JOIN customer AS c
  ON n.n_nationkey = c.c_nationkey
LEFT JOIN orders AS o
  ON c.c_custkey = o.o_custkey
LEFT JOIN lineitem_summary AS ls
  ON o.o_orderkey = ls.l_orderkey
LEFT JOIN top_suppliers AS s
  ON s.s_suppkey = (
    SELECT
      ps.ps_suppkey
    FROM partsupp AS ps
    JOIN lineitem AS l
      ON l.l_partkey = ps.ps_partkey
    WHERE
      l.l_orderkey = o.o_orderkey
    ORDER BY
      ps.ps_supplycost * ps.ps_availqty DESC
    LIMIT 1
  )
WHERE
  o.o_orderdate BETWEEN '1997-01-01' AND '1997-12-31'
GROUP BY
  n.n_name,
  r.r_name
HAVING
  NOT SUM(ls.total_revenue) IS NULL OR COUNT(DISTINCT o.o_orderkey) > 0
ORDER BY
  total_revenue DESC,
  order_count DESC;
