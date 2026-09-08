SELECT
  l.l_orderkey,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS revenue,
  SUM(l.l_quantity) AS quantity_sold,
  AVG(l.l_tax) AS average_tax
FROM lineitem AS l
JOIN orders AS o
  ON l.l_orderkey = o.o_orderkey
JOIN customer AS c
  ON o.o_custkey = c.c_custkey
JOIN supplier AS s
  ON l.l_suppkey = s.s_suppkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey AND s.s_suppkey = ps.ps_suppkey
JOIN part AS p
  ON ps.ps_partkey = p.p_partkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  r.r_name = 'ASIA'
  AND o.o_orderdate BETWEEN CAST('1994-01-01' AS DATE) AND CAST('1994-12-31' AS DATE)
GROUP BY
  l.l_orderkey
ORDER BY
  revenue DESC
LIMIT 10;
