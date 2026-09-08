WITH RankedSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supplycost,
    RANK() OVER (PARTITION BY s.s_nationkey ORDER BY SUM(ps.ps_supplycost * ps.ps_availqty) DESC) AS rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_nationkey
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    c.c_name,
    COUNT(o.o_orderkey) AS order_count,
    SUM(o.o_totalprice) AS total_spent,
    RANK() OVER (ORDER BY SUM(o.o_totalprice) DESC) AS rank
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  WHERE
    o.o_orderdate >= '1997-01-01'
  GROUP BY
    c.c_custkey,
    c.c_name
), NationPerformance AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    COALESCE(SUM(CASE WHEN l.l_returnflag = 'R' THEN l.l_extendedprice END), 0) AS total_returns,
    COUNT(DISTINCT o.o_orderkey) AS completed_orders,
    AVG(o.o_totalprice) AS avg_order_value
  FROM nation AS n
  LEFT JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  LEFT JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  LEFT JOIN lineitem AS l
    ON ps.ps_partkey = l.l_partkey
  LEFT JOIN orders AS o
    ON l.l_orderkey = o.o_orderkey
  GROUP BY
    n.n_nationkey,
    n.n_name
)
SELECT
  r.s_name,
  c.c_name,
  n.n_name,
  n.total_returns,
  n.completed_orders,
  n.avg_order_value,
  RANK() OVER (ORDER BY n.total_returns DESC) AS return_rank
FROM RankedSuppliers AS r
JOIN CustomerOrders AS c
  ON r.s_suppkey = c.c_custkey
JOIN NationPerformance AS n
  ON r.s_suppkey = n.n_nationkey
WHERE
  n.completed_orders > 0
ORDER BY
  return_rank,
  r.s_name;
