WITH ranked_supplier AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    COUNT(ps.ps_partkey) AS total_parts,
    RANK() OVER (PARTITION BY r.r_regionkey ORDER BY COUNT(ps.ps_partkey) DESC) AS supplier_rank
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    r.r_regionkey
), top_suppliers AS (
  SELECT
    supplier_rank,
    s_suppkey,
    s_name,
    s_acctbal
  FROM ranked_supplier
  WHERE
    supplier_rank <= 3
)
SELECT
  c.c_custkey,
  c.c_name,
  o.o_orderkey,
  o.o_orderdate,
  SUM(l.l_extendedprice * (
    1 - l.l_discount
  )) AS total_revenue,
  ts.s_name AS top_supplier_name
FROM customer AS c
JOIN orders AS o
  ON c.c_custkey = o.o_custkey
JOIN lineitem AS l
  ON o.o_orderkey = l.l_orderkey
JOIN partsupp AS ps
  ON l.l_partkey = ps.ps_partkey
JOIN top_suppliers AS ts
  ON ps.ps_suppkey = ts.s_suppkey
WHERE
  o.o_orderdate >= CAST('1997-01-01' AS DATE)
  AND o.o_orderdate < CAST('1997-12-31' AS DATE)
GROUP BY
  c.c_custkey,
  c.c_name,
  o.o_orderkey,
  o.o_orderdate,
  ts.s_name
ORDER BY
  total_revenue DESC,
  c.c_name;
