WITH SupplierStats AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_cost,
    COUNT(DISTINCT ps.ps_partkey) AS part_count,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY SUM(ps.ps_supplycost * ps.ps_availqty) DESC) AS rank_in_nation
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_nationkey
), HighCostSuppliers AS (
  SELECT
    s.s_nationkey,
    s.s_name,
    s.s_address,
    ss.total_cost
  FROM SupplierStats AS ss
  JOIN supplier AS s
    ON ss.s_suppkey = s.s_suppkey
  WHERE
    ss.total_cost > (
      SELECT
        AVG(total_cost)
      FROM SupplierStats
    )
)
SELECT
  n.n_name AS nation_name,
  COUNT(DISTINCT h.s_name) AS high_cost_supplier_count,
  SUM(COALESCE(h.total_cost, 0)) AS total_expenditure,
  AVG(h.total_cost) AS avg_expenditure_per_supplier
FROM nation AS n
LEFT JOIN HighCostSuppliers AS h
  ON n.n_nationkey = h.s_nationkey
GROUP BY
  n.n_name
HAVING
  COUNT(DISTINCT h.s_name) > 0
ORDER BY
  total_expenditure DESC;
