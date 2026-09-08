SELECT
  p.p_name,
  s.s_name,
  c.c_name,
  SUM(l.l_quantity) AS total_quantity,
  AVG(l.l_extendedprice) AS avg_extended_price,
  MAX(l.l_discount) AS max_discount,
  STRING_AGG(CONCAT(l.l_comment, ' (Order: ', o.o_orderkey, ')'), '; ') AS detailed_comments
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
  p.p_name LIKE '%widget%'
GROUP BY
  p.p_name,
  s.s_name,
  c.c_name
HAVING
  SUM(l.l_quantity) > 100
ORDER BY
  total_quantity DESC;
