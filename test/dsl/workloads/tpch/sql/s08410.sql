WITH RegionSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    r.r_name,
    AVG(ps.ps_supplycost) AS avg_supply_cost
  FROM supplier AS s
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  WHERE
    r.r_name IN ('ASIA', 'EUROPE')
  GROUP BY
    s.s_suppkey,
    s.s_name,
    r.r_name
), HighValueOrders AS (
  SELECT
    o.o_orderkey,
    o.o_totalprice,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS final_price
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  GROUP BY
    o.o_orderkey,
    o.o_totalprice
  HAVING
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) > 10000
)
SELECT
  rs.s_suppkey,
  rs.s_name,
  rs.r_name,
  hvo.o_orderkey,
  hvo.final_price,
  rs.avg_supply_cost
FROM RegionSuppliers AS rs
JOIN HighValueOrders AS hvo
  ON rs.s_suppkey = (
    SELECT
      ps.ps_suppkey
    FROM partsupp AS ps
    WHERE
      ps.ps_partkey IN (
        SELECT
          p.p_partkey
        FROM part AS p
        WHERE
          p.p_retailprice > 500.00
      )
    ORDER BY
      ps.ps_supplycost DESC
    LIMIT 1
  )
ORDER BY
  rs.r_name,
  hvo.final_price DESC;
