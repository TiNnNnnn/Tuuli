SELECT
  p.p_name,
  s.s_name,
  ps.ps_supplycost
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
WHERE
  ps.ps_availqty > 100
ORDER BY
  ps.ps_supplycost DESC;
