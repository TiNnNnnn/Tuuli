WITH RegionalSales AS (
  SELECT
    r.r_name AS region_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM region AS r
  JOIN nation AS n
    ON r.r_regionkey = n.n_regionkey
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  WHERE
    o.o_orderdate BETWEEN CAST('1996-01-01' AS DATE) AND CAST('1996-12-31' AS DATE)
  GROUP BY
    r.r_name
), CustomerSegmentation AS (
  SELECT
    c.c_mktsegment,
    COUNT(DISTINCT o.o_orderkey) AS order_count,
    SUM(o.o_totalprice) AS segment_revenue
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderstatus = 'F'
  GROUP BY
    c.c_mktsegment
)
SELECT
  rs.region_name,
  rs.total_sales,
  cs.c_mktsegment,
  cs.order_count,
  cs.segment_revenue
FROM RegionalSales AS rs
JOIN CustomerSegmentation AS cs
  ON cs.segment_revenue > 10000
ORDER BY
  rs.total_sales DESC,
  cs.order_count DESC;
