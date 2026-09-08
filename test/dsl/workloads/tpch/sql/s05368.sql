WITH SupplierOrderCounts AS (
  SELECT
    s.s_suppkey,
    COUNT(DISTINCT o.o_orderkey) AS order_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN lineitem AS l
    ON ps.ps_partkey = l.l_partkey
  JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  WHERE
    o.o_orderstatus = 'O'
  GROUP BY
    s.s_suppkey
), TopSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    soc.order_count
  FROM supplier AS s
  JOIN SupplierOrderCounts AS soc
    ON s.s_suppkey = soc.s_suppkey
  ORDER BY
    soc.order_count DESC
  LIMIT 5
), PartDetails AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_mfgr,
    p.p_brand,
    p.p_retailprice
  FROM part AS p
  WHERE
    p.p_retailprice > (
      SELECT
        AVG(p2.p_retailprice)
      FROM part AS p2
    )
)
SELECT
  ts.s_name,
  ts.order_count,
  pd.p_name,
  pd.p_mfgr,
  pd.p_brand,
  pd.p_retailprice
FROM TopSuppliers AS ts
JOIN lineitem AS l
  ON ts.s_suppkey = l.l_suppkey
JOIN PartDetails AS pd
  ON l.l_partkey = pd.p_partkey
WHERE
  l.l_shipmode = 'REG AIR' AND l.l_shipdate BETWEEN '1997-01-01' AND '1997-12-31'
ORDER BY
  ts.order_count DESC,
  pd.p_retailprice DESC;
