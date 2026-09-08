WITH RECURSIVE supplier_hierarchy AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    1 AS hierarchy_level
  FROM supplier AS s
  WHERE
    s.s_acctbal >= (
      SELECT
        AVG(s_acctbal)
      FROM supplier
    )
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    sh.hierarchy_level + 1
  FROM supplier AS s
  JOIN supplier_hierarchy AS sh
    ON s.s_nationkey = sh.s_suppkey
  WHERE
    sh.hierarchy_level < 5
), high_value_orders AS (
  SELECT
    o.o_orderkey,
    o.o_totalprice,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS rn
  FROM orders AS o
  WHERE
    o.o_totalprice > (
      SELECT
        AVG(o_totalprice)
      FROM orders
    )
), part_supplier_counts AS (
  SELECT
    ps.ps_partkey,
    COUNT(*) AS supplier_count
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
), national_sales AS (
  SELECT
    n.n_nationkey,
    SUM(o.o_totalprice) AS total_sales
  FROM nation AS n
  LEFT JOIN customer AS c
    ON n.n_nationkey = c.c_nationkey
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    n.n_nationkey
)
SELECT
  r.r_name,
  ps.part_count,
  COALESCE(ns.total_sales, 0) AS total_sales,
  sh.s_name,
  sh.hierarchy_level
FROM region AS r
LEFT JOIN (
  SELECT
    p.p_partkey,
    COUNT(*) AS part_count
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  GROUP BY
    p.p_partkey
) AS ps
  ON r.r_regionkey = ps.p_partkey
FULL OUTER JOIN national_sales AS ns
  ON ns.n_nationkey = r.r_regionkey
INNER JOIN supplier_hierarchy AS sh
  ON sh.s_suppkey = r.r_regionkey
WHERE
  (
    r.r_name LIKE '%East%' OR r.r_name IS NULL
  )
  AND ps.part_count > (
    SELECT
      AVG(part_count)
    FROM (
      SELECT
        p.p_partkey,
        COUNT(*) AS part_count
      FROM part AS p
      JOIN partsupp AS ps
        ON p.p_partkey = ps.ps_partkey
      GROUP BY
        p.p_partkey
    ) AS temp
  )
ORDER BY
  total_sales DESC,
  sh.hierarchy_level ASC;
