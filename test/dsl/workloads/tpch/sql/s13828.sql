SELECT
  p.p_partkey,
  p.p_name,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue
FROM part AS p
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
WHERE
  o.o_orderdate BETWEEN CAST('1997-01-01' AS DATE) AND CAST('1997-12-31' AS DATE)
GROUP BY
  p.p_partkey,
  p.p_name
ORDER BY
  total_revenue DESC
LIMIT 10;
