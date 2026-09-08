SELECT
  c.c_name AS customer_name,
  s.s_name AS supplier_name,
  p.p_name AS part_name,
  SUM(l.l_quantity) AS total_quantity,
  COUNT(DISTINCT o.o_orderkey) AS order_count,
  CONCAT('Supply from ', s.s_name, ' for part ', p.p_name, ' requested by customer ', c.c_name) AS benchmark_message
FROM customer AS c
JOIN orders AS o
  ON c.c_custkey = o.o_custkey
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN part AS p
  ON ps.ps_partkey = p.p_partkey
WHERE
  c.c_mktsegment = 'BUILDING'
  AND l.l_shipdate >= CAST('1997-01-01' AS DATE)
  AND l.l_shipdate < CAST('1998-01-01' AS DATE)
GROUP BY
  c.c_name,
  s.s_name,
  p.p_name
HAVING
  SUM(l.l_quantity) > 100
ORDER BY
  total_quantity DESC;
