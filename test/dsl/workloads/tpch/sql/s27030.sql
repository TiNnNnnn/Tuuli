SELECT
  p.p_name,
  COUNT(DISTINCT s.s_suppkey) AS supplier_count,
  AVG(ps.ps_supplycost) AS avg_supplycost,
  STRING_AGG(DISTINCT n.n_name, ', ') AS nations_supplied,
  RANK() OVER (ORDER BY AVG(ps.ps_supplycost) DESC) AS supply_rank
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
WHERE
  p.p_brand LIKE '%BrandA%' AND LENGTH(p.p_comment) > 10 AND NOT n.n_name LIKE 'N%'
GROUP BY
  p.p_name
HAVING
  COUNT(DISTINCT s.s_suppkey) > 5
ORDER BY
  supply_rank;
