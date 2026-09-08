WITH RECURSIVE CustomerHierarchy AS (
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_acctbal,
    1 AS depth
  FROM customer AS c
  WHERE
    c.c_acctbal > 1000
  UNION ALL
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_acctbal,
    ch.depth + 1
  FROM customer AS c
  JOIN CustomerHierarchy AS ch
    ON c.c_nationkey = (
      SELECT
        n.n_nationkey
      FROM nation AS n
      WHERE
        n.n_name = 'Germany'
    )
  WHERE
    c.c_acctbal < ch.c_acctbal
), RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) DESC) AS order_rank
  FROM orders AS o
  JOIN lineitem AS l
    ON o.o_orderkey = l.l_orderkey
  WHERE
    l.l_shipdate >= '1997-01-01' AND l.l_shipdate < '1998-01-01'
  GROUP BY
    o.o_orderkey,
    o.o_orderdate,
    o.o_orderstatus
), SupplierPartDetails AS (
  SELECT
    s.s_suppkey,
    p.p_name,
    ps.ps_availqty,
    ps.ps_supplycost,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY ps.ps_supplycost DESC) AS rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  WHERE
    ps.ps_availqty > 0
)
SELECT
  ch.c_name,
  ch.c_acctbal,
  r.o_orderkey,
  r.o_orderdate,
  r.total_sales,
  COALESCE(sp.p_name, 'No Parts Available') AS part_name,
  sp.ps_availqty,
  sp.ps_supplycost,
  CASE
    WHEN r.total_sales > 10000
    THEN 'High Value'
    WHEN r.total_sales BETWEEN 5000 AND 10000
    THEN 'Medium Value'
    ELSE 'Low Value'
  END AS order_value_category
FROM CustomerHierarchy AS ch
LEFT JOIN RankedOrders AS r
  ON ch.c_custkey = r.o_orderkey
LEFT JOIN SupplierPartDetails AS sp
  ON sp.rank = 1
WHERE
  ch.depth <= 3 AND NOT r.o_orderdate IS NULL
ORDER BY
  ch.c_name,
  r.total_sales DESC;
