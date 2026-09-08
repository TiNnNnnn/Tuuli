SELECT
  p.p_partkey,
  p.p_name,
  p.p_mfgr,
  SUM(ps.ps_availqty) AS total_available_quantity,
  SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
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
  r.r_name = 'ASIA'
GROUP BY
  p.p_partkey,
  p.p_name,
  p.p_mfgr
ORDER BY
  total_available_quantity DESC
LIMIT 10;
