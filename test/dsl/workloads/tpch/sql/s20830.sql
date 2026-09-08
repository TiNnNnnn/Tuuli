WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    RANK() OVER (PARTITION BY p.p_type ORDER BY s.s_acctbal DESC) AS supplier_rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  WHERE
    s.s_acctbal > (
      SELECT
        AVG(s2.s_acctbal)
      FROM supplier AS s2
      WHERE
        s2.s_nationkey = s.s_nationkey
    )
)
SELECT
  n.n_name AS nation_name,
  p.p_name AS part_name,
  COALESCE(SUM(l.l_quantity), 0) AS total_quantity,
  AVG(l.l_extendedprice) AS avg_extended_price,
  STRING_AGG(
    DISTINCT CASE WHEN ps.ps_supplycost IS NULL THEN 'Not supplied' ELSE 'Supplied' END,
    ', '
  ) AS supply_status,
  MAX(s.s_acctbal) AS max_supplier_balance
FROM nation AS n
LEFT JOIN customer AS c
  ON c.c_nationkey = n.n_nationkey
LEFT JOIN orders AS o
  ON o.o_custkey = c.c_custkey
LEFT JOIN lineitem AS l
  ON l.l_orderkey = o.o_orderkey
LEFT JOIN partsupp AS ps
  ON ps.ps_partkey = l.l_partkey
LEFT JOIN RankedSuppliers AS s
  ON s.s_suppkey = ps.ps_suppkey AND s.supplier_rank = 1
JOIN part AS p
  ON p.p_partkey = l.l_partkey
WHERE
  (
    NOT n.n_name IS NULL OR NOT p.p_type IS NULL
  )
  AND (
    l.l_returnflag IN ('R', 'N') OR l.l_discount BETWEEN 0 AND 0.1
  )
GROUP BY
  n.n_name,
  p.p_name
HAVING
  SUM(l.l_quantity) > 0
  AND MAX(l.l_discount) IS NOT DISTINCT FROM 0
  AND NOT COUNT(DISTINCT s.s_suppkey) IS NULL
ORDER BY
  p.p_name ASC;
