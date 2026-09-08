WITH RECURSIVE region_supplier AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    s.s_acctbal,
    s.s_comment
  FROM supplier AS s
  INNER JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  WHERE
    n.n_name = 'Canada'
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    s.s_acctbal,
    s.s_comment
  FROM supplier AS s
  INNER JOIN region_supplier AS rs
    ON rs.s_nationkey = s.s_nationkey
  WHERE
    rs.s_acctbal > 5000
), order_summary AS (
  SELECT
    o.o_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    l.l_shipdate >= CAST('1997-01-01' AS DATE)
    AND l.l_shipdate <= CAST('1997-12-31' AS DATE)
  GROUP BY
    o.o_orderkey
), market_analysis AS (
  SELECT
    c.c_mktsegment,
    COUNT(DISTINCT o.o_orderkey) AS order_count,
    SUM(os.total_revenue) AS segment_revenue
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  LEFT JOIN order_summary AS os
    ON o.o_orderkey = os.o_orderkey
  GROUP BY
    c.c_mktsegment
)
SELECT
  r.r_name,
  COALESCE(SUM(ma.segment_revenue), 0) AS total_segment_revenue,
  COUNT(DISTINCT rs.s_suppkey) AS supplier_count,
  AVG(rs.s_acctbal) AS average_acctbal
FROM region AS r
LEFT JOIN nation AS n
  ON r.r_regionkey = n.n_regionkey
LEFT JOIN region_supplier AS rs
  ON n.n_nationkey = rs.s_nationkey
LEFT JOIN market_analysis AS ma
  ON ma.c_mktsegment = 'BUILDING'
GROUP BY
  r.r_name
HAVING
  AVG(rs.s_acctbal) > (
    SELECT
      AVG(s.s_acctbal)
    FROM supplier AS s
    WHERE
      NOT s.s_acctbal IS NULL
  )
ORDER BY
  total_segment_revenue DESC;
