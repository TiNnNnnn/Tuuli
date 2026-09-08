WITH SupplierCost AS (
  SELECT
    s.s_suppkey,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey
), NationSummary AS (
  SELECT
    n.n_nationkey,
    n.n_name,
    SUM(o.o_totalprice) AS total_sales
  FROM nation AS n
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN customer AS c
    ON s.s_suppkey = c.c_custkey
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    n.n_nationkey,
    n.n_name
), PartDetail AS (
  SELECT
    p.p_partkey,
    p.p_name,
    COUNT(l.l_linenumber) AS total_lines,
    AVG(l.l_extendedprice) AS avg_price
  FROM part AS p
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  GROUP BY
    p.p_partkey,
    p.p_name
)
SELECT
  ns.n_name,
  pd.p_name,
  SUM(ns.total_sales) AS total_sales,
  SUM(sc.total_cost) AS total_cost,
  MAX(pd.avg_price) AS max_avg_price
FROM NationSummary AS ns
JOIN SupplierCost AS sc
  ON ns.n_nationkey = sc.s_suppkey
JOIN PartDetail AS pd
  ON pd.p_partkey = sc.s_suppkey
GROUP BY
  ns.n_name,
  pd.p_name
ORDER BY
  total_sales DESC,
  total_cost DESC
LIMIT 10;
