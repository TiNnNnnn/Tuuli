WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    n.n_name AS nation_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_cost,
    DENSE_RANK() OVER (PARTITION BY n.n_name ORDER BY SUM(ps.ps_supplycost * ps.ps_availqty) DESC) AS rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    n.n_name
), TopSuppliers AS (
  SELECT
    nation_name,
    s.s_suppkey,
    s.s_name,
    total_supply_cost
  FROM RankedSuppliers AS rs
  JOIN supplier AS s
    ON s.s_suppkey = rs.s_suppkey
  WHERE
    rs.rank <= 3
)
SELECT
  p.p_partkey,
  p.p_name,
  p.p_brand,
  p.p_retailprice,
  ts.nation_name,
  ts.s_name AS top_supplier,
  ts.total_supply_cost
FROM part AS p
JOIN partsupp AS ps
  ON p.p_partkey = ps.ps_partkey
JOIN TopSuppliers AS ts
  ON ps.ps_suppkey = ts.s_suppkey
WHERE
  p.p_retailprice > (
    SELECT
      AVG(p2.p_retailprice)
    FROM part AS p2
  )
ORDER BY
  ts.nation_name,
  ts.total_supply_cost DESC,
  p.p_retailprice DESC
LIMIT 50;
