WITH PartDetails AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_mfgr,
    p.p_brand,
    p.p_type,
    CONCAT(p.p_name, ' | ', p.p_brand, ' | ', p.p_mfgr) AS full_description
  FROM part AS p
), NationRegion AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    r.r_name AS region_name,
    CONCAT(n.n_name, ' in ', r.r_name) AS nation_region
  FROM nation AS n
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    COUNT(o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_spent
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name
), SupplierParts AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    COUNT(ps.ps_partkey) AS supplied_parts
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
)
SELECT
  pd.full_description,
  nr.nation_region,
  co.c_name,
  co.total_orders,
  co.total_spent,
  sp.s_name,
  sp.supplied_parts
FROM PartDetails AS pd
JOIN SupplierParts AS sp
  ON pd.p_partkey IN (
    SELECT
      ps.ps_partkey
    FROM partsupp AS ps
    WHERE
      ps.ps_suppkey = sp.s_suppkey
  )
JOIN CustomerOrders AS co
  ON co.total_orders > 5
JOIN NationRegion AS nr
  ON (
    nr.n_nationkey IN (
      SELECT
        c.c_nationkey
      FROM customer AS c
      WHERE
        c.c_custkey = co.c_custkey
    )
  )
WHERE
  pd.p_type LIKE '%rubber%'
ORDER BY
  co.total_spent DESC,
  sp.supplied_parts ASC;
