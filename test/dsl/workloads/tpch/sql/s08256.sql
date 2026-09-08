WITH supplier_details AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_nationkey,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_nationkey
), nation_costs AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    SUM(sd.total_supply_cost) AS nation_supply_cost
  FROM supplier_details AS sd
  JOIN nation AS n
    ON sd.s_nationkey = n.n_nationkey
  GROUP BY
    n.n_nationkey,
    n.n_name
), region_costs AS (
  SELECT
    r.r_regionkey,
    r.r_name,
    SUM(nc.nation_supply_cost) AS region_supply_cost
  FROM nation_costs AS nc
  JOIN nation AS n
    ON nc.n_nationkey = n.n_nationkey
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  GROUP BY
    r.r_regionkey,
    r.r_name
)
SELECT
  r.r_name,
  r.region_supply_cost,
  COUNT(DISTINCT s.s_suppkey) AS number_of_suppliers
FROM region_costs AS r
JOIN supplier AS s
  ON s.s_nationkey IN (
    SELECT
      n_nationkey
    FROM nation AS n
    JOIN region AS r
      ON n.n_regionkey = r.r_regionkey
    WHERE
      r.r_regionkey = r.r_regionkey
  )
GROUP BY
  r.r_name,
  r.region_supply_cost
ORDER BY
  r.region_supply_cost DESC,
  number_of_suppliers DESC
LIMIT 10;
