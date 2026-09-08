SELECT
  LOWER(SUBSTRING(p.p_name FROM 1 FOR 10)) AS truncated_name,
  REPLACE(p.p_comment, 'old', 'new') AS modified_comment,
  CONCAT(s.s_name, ' from ', n.n_name) AS supplier_details,
  STRING_AGG(DISTINCT CONCAT(c.c_name, ' (', c.c_mktsegment, ')'), '; ') AS customer_info,
  COUNT(DISTINCT o.o_orderkey) AS total_orders,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
WHERE
  p.p_name LIKE '%widget%' AND c.c_acctbal > 1000
GROUP BY
  p.p_name,
  p.p_comment,
  s.s_name,
  n.n_name
HAVING
  COUNT(DISTINCT o.o_orderkey) > 5
ORDER BY
  total_revenue DESC;
