WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    o.o_orderstatus,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS order_rank
  FROM orders AS o
  WHERE
    o.o_orderdate >= CURRENT_DATE - INTERVAL '1 YEAR'
), SupplierPartDetails AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    p.p_partkey,
    p.p_brand,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_value
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    p.p_partkey,
    p.p_brand
), TopSuppliers AS (
  SELECT
    s.s_name,
    s.total_supply_value,
    RANK() OVER (ORDER BY s.total_supply_value DESC) AS supplier_rank
  FROM SupplierPartDetails AS s
  WHERE
    s.total_supply_value > (
      SELECT
        AVG(total_supply_value)
      FROM SupplierPartDetails
    )
)
SELECT
  o.o_orderkey,
  o.o_orderdate,
  o.o_totalprice,
  o.o_orderstatus,
  tp.s_name AS top_supplier,
  COALESCE(lp.total_line_items, 0) AS total_line_items,
  COALESCE(total_nations.supplier_nation_count, 0) AS nation_count
FROM RankedOrders AS o
LEFT JOIN (
  SELECT
    l.l_orderkey,
    COUNT(*) AS total_line_items
  FROM lineitem AS l
  GROUP BY
    l.l_orderkey
) AS lp
  ON o.o_orderkey = lp.l_orderkey
LEFT JOIN (
  SELECT
    n.n_nationkey,
    COUNT(s.s_suppkey) AS supplier_nation_count
  FROM supplier AS s
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  GROUP BY
    n.n_nationkey
) AS total_nations
  ON total_nations.n_nationkey = o.o_orderkey % 5
JOIN TopSuppliers AS tp
  ON (
    o.o_totalprice > 1000 OR o.o_orderstatus = 'F'
  ) AND tp.supplier_rank <= 5
WHERE
  o.o_orderstatus IN ('O', 'F')
ORDER BY
  o.o_orderdate DESC,
  o.o_totalprice DESC;
