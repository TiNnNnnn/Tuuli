SELECT
  p.p_brand,
  p.p_type,
  COUNT(DISTINCT ps.ps_suppkey) AS supplier_count,
  AVG(ps.ps_supplycost) AS avg_supply_cost,
  SUM(p.p_retailprice) AS total_retail_price,
  STRING_AGG(DISTINCT s.s_name, ', ') AS supplier_names
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
  r.r_name LIKE '%Asia%' AND p.p_size BETWEEN 10 AND 20
GROUP BY
  p.p_brand,
  p.p_type
HAVING
  COUNT(DISTINCT ps.ps_suppkey) > 5
ORDER BY
  avg_supply_cost DESC,
  total_retail_price ASC;
