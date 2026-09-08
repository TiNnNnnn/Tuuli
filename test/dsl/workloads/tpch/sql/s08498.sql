WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    c.c_mktsegment,
    ROW_NUMBER() OVER (PARTITION BY c.c_mktsegment ORDER BY o.o_totalprice DESC) AS rn
  FROM orders AS o
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
), TopOrderSegments AS (
  SELECT
    ro.c_mktsegment,
    ro.o_orderkey,
    ro.o_orderdate,
    ro.o_totalprice
  FROM RankedOrders AS ro
  WHERE
    ro.rn <= 10
)
SELECT
  p.p_name,
  SUM(l.l_quantity) AS total_quantity,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  n.n_name AS supplier_nation
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN TopOrderSegments AS tos
  ON o.o_orderkey = tos.o_orderkey
JOIN part AS p
  ON l.l_partkey = p.p_partkey
WHERE
  l.l_shipdate >= CAST('1996-01-01' AS DATE)
  AND l.l_shipdate < CAST('1997-01-01' AS DATE)
GROUP BY
  p.p_name,
  n.n_name
HAVING
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) > 10000
ORDER BY
  revenue DESC,
  total_quantity DESC;
