SELECT
  CONCAT(c.c_name, ' from ', s.s_name, ' in ', n.n_name, ' supplies ', p.p_name) AS supply_info,
  LENGTH(CONCAT(c.c_name, ' from ', s.s_name, ' in ', n.n_name, ' supplies ', p.p_name)) AS info_length,
  SUBSTRING(CONCAT(c.c_name, ' from ', s.s_name, ' in ', n.n_name, ' supplies ', p.p_name) FROM 1 FOR 50) AS short_supply_info
FROM customer AS c
JOIN orders AS o
  ON c.c_custkey = o.o_custkey
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey
JOIN supplier AS s
  ON ps.ps_suppkey = s.s_suppkey
JOIN nation AS n
  ON s.s_nationkey = n.n_nationkey
JOIN part AS p
  ON ps.ps_partkey = p.p_partkey
WHERE
  LENGTH(s.s_comment) > 50 AND o.o_orderstatus = 'O'
ORDER BY
  info_length DESC
LIMIT 10;
