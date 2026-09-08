WITH SupplierOrderInfo AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    n.n_name AS nation_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    COUNT(DISTINCT o.o_orderkey) AS order_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
    AND o.o_orderdate < CAST('1998-01-01' AS DATE)
  GROUP BY
    s.s_suppkey,
    s.s_name,
    n.n_name
), RankedSuppliers AS (
  SELECT
    soi.nation_name,
    soi.s_name,
    soi.total_revenue,
    soi.order_count,
    RANK() OVER (PARTITION BY soi.nation_name ORDER BY soi.total_revenue DESC) AS revenue_rank
  FROM SupplierOrderInfo AS soi
)
SELECT
  r.nation_name,
  rs.s_name,
  rs.total_revenue,
  rs.order_count
FROM RankedSuppliers AS rs
JOIN (
  SELECT DISTINCT
    n_name AS nation_name
  FROM nation
) AS r
  ON rs.nation_name = r.nation_name
WHERE
  rs.revenue_rank <= 5
ORDER BY
  r.nation_name,
  rs.total_revenue DESC;
