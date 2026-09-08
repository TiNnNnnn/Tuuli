WITH RankedSupplier AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    ROW_NUMBER() OVER (PARTITION BY s.s_nationkey ORDER BY s.s_acctbal DESC) AS rank
  FROM supplier AS s
), CustomerSummary AS (
  SELECT
    c.c_custkey,
    c.c_name,
    SUM(o.o_totalprice) AS total_spent,
    AVG(o.o_totalprice) AS avg_order_value,
    COUNT(o.o_orderkey) AS order_count
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name
), PartSupplierInfo AS (
  SELECT
    p.p_partkey,
    p.p_name,
    SUM(ps.ps_availqty) AS total_available,
    AVG(ps.ps_supplycost) AS average_cost
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  GROUP BY
    p.p_partkey,
    p.p_name
)
SELECT
  cs.c_name,
  pi.p_name,
  pi.total_available,
  pi.average_cost,
  cs.total_spent,
  COALESCE(s.s_name, 'Unknown Supplier') AS supplier_name
FROM CustomerSummary AS cs
JOIN lineitem AS l
  ON cs.c_custkey = l.l_orderkey
JOIN PartSupplierInfo AS pi
  ON l.l_partkey = pi.p_partkey
LEFT JOIN RankedSupplier AS s
  ON s.rank = 1 AND s.s_suppkey = l.l_suppkey
WHERE
  cs.total_spent > (
    SELECT
      AVG(total_spent)
    FROM CustomerSummary
  )
  AND pi.total_available < 100
  AND NOT pi.average_cost IS NULL
ORDER BY
  cs.total_spent DESC,
  pi.average_cost ASC;
