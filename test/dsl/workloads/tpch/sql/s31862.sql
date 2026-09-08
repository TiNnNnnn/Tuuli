WITH RECURSIVE SupplierHierarchy AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    s.s_nationkey,
    0 AS level
  FROM supplier AS s
  WHERE
    s.s_acctbal > (
      SELECT
        AVG(s_acctbal)
      FROM supplier
    )
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    s.s_nationkey,
    sh.level + 1
  FROM supplier AS s
  JOIN SupplierHierarchy AS sh
    ON s.s_nationkey = sh.s_nationkey
  WHERE
    sh.level < 10
), RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    c.c_mktsegment,
    ROW_NUMBER() OVER (PARTITION BY c.c_mktsegment ORDER BY o.o_totalprice DESC) AS rnk
  FROM orders AS o
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
), PartSupply AS (
  SELECT
    ps.ps_partkey,
    SUM(ps.ps_availqty * ps.ps_supplycost) AS total_supply_cost
  FROM partsupp AS ps
  GROUP BY
    ps.ps_partkey
), MaxSupply AS (
  SELECT
    MAX(total_supply_cost) AS max_supply
  FROM PartSupply
), OrderDetails AS (
  SELECT
    li.l_orderkey,
    p.p_partkey,
    p.p_name,
    li.l_quantity,
    li.l_extendedprice,
    li.l_discount,
    (
      li.l_extendedprice * (
        1 - li.l_discount
      )
    ) AS net_price
  FROM lineitem AS li
  JOIN part AS p
    ON li.l_partkey = p.p_partkey
)
SELECT DISTINCT
  r.o_orderkey,
  r.o_orderdate,
  o.p_partkey,
  o.p_name,
  sh.s_name,
  o.net_price
FROM RankedOrders AS r
JOIN OrderDetails AS o
  ON r.o_orderkey = o.l_orderkey
LEFT JOIN supplier AS sh
  ON sh.s_suppkey = (
    SELECT
      ps.ps_suppkey
    FROM partsupp AS ps
    WHERE
      ps.ps_partkey = o.p_partkey
    ORDER BY
      ps.ps_supplycost ASC
    LIMIT 1
  )
WHERE
  r.rnk = 1
  AND EXISTS(
    SELECT
      1
    FROM nation AS n
    WHERE
      n.n_nationkey = sh.s_nationkey AND n.n_name LIKE 'A%'
  )
  AND o.net_price > (
    SELECT
      COALESCE(MAX(m.max_supply), 0)
    FROM MaxSupply AS m
  )
ORDER BY
  r.o_orderdate DESC,
  o.p_partkey;
