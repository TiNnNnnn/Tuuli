SELECT
  p.p_name AS part_name,
  s.s_name AS supplier_name,
  c.c_name AS customer_name,
  o.o_orderkey AS order_key,
  o.o_orderdate AS order_date,
  COUNT(l.l_orderkey) AS total_lines,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  SUBSTRING(p.p_comment FROM 1 FOR 20) AS short_comment,
  CONCAT('Order: ', o.o_orderkey, ' | Date: ', o.o_orderdate) AS order_details
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  p.p_size > 10
  AND c.c_mktsegment = 'BUILDING'
  AND o.o_orderdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name,
  o.o_orderkey,
  o.o_orderdate,
  p.p_comment
ORDER BY
  total_revenue DESC,
  order_date ASC
LIMIT 50;
