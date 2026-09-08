SELECT
  p.p_brand,
  p.p_type,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue
FROM part AS p
JOIN lineitem AS l
  ON p.p_partkey = l.l_partkey
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  r.r_name = 'ASIA'
  AND l.l_shipdate >= CAST('1994-01-01' AS DATE)
  AND l.l_shipdate < CAST('1995-01-01' AS DATE)
GROUP BY
  p.p_brand,
  p.p_type
ORDER BY
  revenue DESC;
