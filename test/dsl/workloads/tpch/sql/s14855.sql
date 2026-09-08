SELECT
  p.p_partkey,
  p.p_name,
  p.p_mfgr,
  SUM(ps.ps_availqty) AS total_availqty,
  AVG(ps.ps_supplycost) AS avg_supplycost
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN region AS r
  ON n.n_regionkey = r.r_regionkey
WHERE
  r.r_name = 'Asia'
GROUP BY
  p.p_partkey,
  p.p_name,
  p.p_mfgr
ORDER BY
  total_availqty DESC
LIMIT 10;
