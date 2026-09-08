WITH RECURSIVE order_summary AS (
  SELECT
    c.c_custkey,
    c.c_name,
    SUM(o.o_totalprice) AS total_spent,
    COUNT(o.o_orderkey) AS total_orders
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name
  HAVING
    COUNT(o.o_orderkey) > 0
), supplier_ranked AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    RANK() OVER (PARTITION BY ps_partkey ORDER BY ps_supplycost DESC) AS rank,
    p.p_name
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  WHERE
    ps_availqty > 0
), high_value_orders AS (
  SELECT
    o.o_orderkey,
    o.o_totalprice,
    RANK() OVER (ORDER BY o.o_totalprice DESC) AS price_rank
  FROM orders AS o
  WHERE
    o.o_totalprice > (
      SELECT
        AVG(o1.o_totalprice)
      FROM orders AS o1
    )
)
SELECT
  r.r_name,
  s.s_name,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  SUM(o.o_totalprice) AS total_revenue,
  AVG(l.l_orderkey) AS avg_order_num_per_lineitem,
  COALESCE(SUM(l.l_discount), 0) AS total_discount_applied
FROM region AS r
JOIN nation AS n
  ON r.r_regionkey = n.n_regionkey
JOIN supplier AS s
  ON n.n_nationkey = s.s_nationkey
LEFT JOIN partsupp AS ps
  ON s.s_suppkey = ps.ps_suppkey
LEFT JOIN lineitem AS l
  ON ps.ps_partkey = l.l_partkey
LEFT JOIN high_value_orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  NOT r.r_name IS NULL AND (
    s.s_name LIKE 'Supplier%' OR s.s_name IS NULL
  )
GROUP BY
  r.r_name,
  s.s_name
ORDER BY
  total_revenue DESC,
  order_count DESC;
