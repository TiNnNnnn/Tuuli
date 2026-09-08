WITH RECURSIVE HighValueSuppliers AS (
  SELECT
    s_suppkey,
    s_name,
    s_acctbal
  FROM supplier
  WHERE
    s_acctbal > (
      SELECT
        AVG(s_acctbal)
      FROM supplier
    )
  UNION ALL
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal
  FROM supplier AS s
  JOIN HighValueSuppliers AS h
    ON s.s_suppkey = h.s_suppkey
  WHERE
    NOT s.s_acctbal IS NULL
), RankedParts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    ROW_NUMBER() OVER (PARTITION BY p.p_type ORDER BY p.p_retailprice DESC) AS rank,
    NULLIF(p.p_comment, '') AS effective_comment
  FROM part AS p
  WHERE
    p.p_size BETWEEN 10 AND 30
), SupplierLineitems AS (
  SELECT
    l.l_orderkey,
    l.l_partkey,
    l.l_suppkey,
    l.l_quantity * (
      1 - l.l_discount
    ) AS effective_price,
    ROW_NUMBER() OVER (PARTITION BY l.l_orderkey ORDER BY l.l_linestatus DESC) AS item_order
  FROM lineitem AS l
  JOIN partsupp AS ps
    ON l.l_partkey = ps.ps_partkey
  WHERE
    ps.ps_availqty > 0 AND l.l_returnflag = 'N'
), FinalComparison AS (
  SELECT
    r.r_name AS region_name,
    COUNT(DISTINCT c.c_custkey) AS customer_count,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_revenue,
    AVG(l.l_tax) AS average_tax_rate
  FROM orders AS o
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
  JOIN supplier AS s
    ON s.s_suppkey = (
      SELECT
        ps.ps_suppkey
      FROM partsupp AS ps
      WHERE
        ps.ps_partkey IN (
          SELECT
            p.p_partkey
          FROM RankedParts AS p
          WHERE
            p.rank = 1
        )
      ORDER BY
        ps.ps_supplycost
      LIMIT 1
    )
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  JOIN nation AS n
    ON c.c_nationkey = n.n_nationkey
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  WHERE
    o.o_orderstatus = 'F' AND o.o_orderdate BETWEEN '1997-01-01' AND '1997-12-31'
  GROUP BY
    r.r_name
)
SELECT
  region_name,
  customer_count,
  total_revenue,
  average_tax_rate
FROM FinalComparison
WHERE
  total_revenue > (
    SELECT
      AVG(total_revenue)
    FROM FinalComparison
  )
ORDER BY
  total_revenue DESC
LIMIT 10;
