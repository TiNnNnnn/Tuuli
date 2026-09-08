SELECT
  p.p_name AS part_name,
  s.s_name AS supplier_name,
  c.c_name AS customer_name,
  o.o_orderkey AS order_id,
  SUM(l.l_quantity) AS total_quantity,
  SUM(l.l_extendedprice) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  MAX(l.l_shipdate) AS latest_ship_date,
  MIN(l.l_shipdate) AS earliest_ship_date,
  STRING_AGG(DISTINCT l.l_comment, '; ') AS comments_aggregated
FROM part AS p
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN supplier AS s
  ON l.l_suppkey = s.s_suppkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  p.p_retailprice BETWEEN 10.00 AND 100.00
  AND c.c_mktsegment = 'BUILDING'
  AND l.l_shipdate >= CAST('1997-01-01' AS DATE)
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name,
  o.o_orderkey
ORDER BY
  total_revenue DESC,
  part_name ASC;
