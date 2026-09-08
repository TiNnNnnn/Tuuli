SELECT
  p.p_name,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  COUNT(DISTINCT o.o_orderkey) AS order_count
FROM part AS p
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  o.o_orderdate >= '1997-01-01' AND o.o_orderdate < '1998-01-01'
GROUP BY
  p.p_name
ORDER BY
  total_revenue DESC
LIMIT 10;
