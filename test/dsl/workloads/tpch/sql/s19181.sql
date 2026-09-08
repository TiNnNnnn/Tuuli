SELECT
  s.s_name,
  COUNT(*) AS total_parts
FROM supplier AS s
JOIN partsupp AS ps
  ON s.s_suppkey = ps.ps_suppkey
GROUP BY
  s.s_name
ORDER BY
  total_parts DESC
LIMIT 10;
