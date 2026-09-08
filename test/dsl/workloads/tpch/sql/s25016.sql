WITH RankedSuppliers AS (
  SELECT
    s.s_name,
    p.p_name,
    ps.ps_supplycost,
    ROW_NUMBER() OVER (PARTITION BY p.p_partkey ORDER BY ps.ps_supplycost ASC) AS supplier_rank
  FROM partsupp AS ps
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
), TopSuppliers AS (
  SELECT
    r.r_name,
    rs.s_name,
    rs.p_name,
    rs.ps_supplycost
  FROM RankedSuppliers AS rs
  JOIN nation AS n
    ON n.n_nationkey = (
      SELECT
        s_nationkey
      FROM supplier AS s
      WHERE
        s.s_name = rs.s_name
      LIMIT 1
    )
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  WHERE
    rs.supplier_rank <= 3
)
SELECT
  r_name,
  STRING_AGG(CONCAT(s_name, ' - ', p_name, ' ($', ps_supplycost, ')'), '; ') AS supplier_info
FROM TopSuppliers
GROUP BY
  r_name
ORDER BY
  r_name;
