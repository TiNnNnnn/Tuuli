WITH RECURSIVE order_hierarchy AS (
  SELECT
    o.o_orderkey,
    o.o_custkey,
    o.o_orderdate,
    o.o_orderpriority,
    1 AS depth
  FROM orders AS o
  WHERE
    o.o_orderdate >= CAST('1997-01-01' AS DATE)
  UNION ALL
  SELECT
    o.o_orderkey,
    o.o_custkey,
    o.o_orderdate,
    o.o_orderpriority,
    oh.depth + 1
  FROM orders AS o
  JOIN order_hierarchy AS oh
    ON o.o_custkey = oh.o_custkey
  WHERE
    o.o_orderdate > oh.o_orderdate
), supplier_summary AS (
  SELECT
    s.s_nationkey,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supply_value,
    COUNT(s.s_suppkey) AS supplier_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_nationkey
), lineitem_summary AS (
  SELECT
    l.l_orderkey,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_price,
    AVG(l.l_quantity) AS avg_quantity,
    COUNT(CASE WHEN l.l_returnflag = 'R' THEN 1 END) AS returned_count
  FROM lineitem AS l
  GROUP BY
    l.l_orderkey
)
SELECT
  n.n_name,
  COALESCE(ss.total_supply_value, 0) AS total_supply_value,
  COALESCE(ss.supplier_count, 0) AS supplier_count,
  oh.o_orderkey,
  oh.depth,
  ls.total_price,
  ls.avg_quantity,
  ls.returned_count,
  DENSE_RANK() OVER (PARTITION BY n.n_name ORDER BY ls.total_price DESC) AS price_rank
FROM nation AS n
LEFT JOIN supplier_summary AS ss
  ON n.n_nationkey = ss.s_nationkey
LEFT JOIN order_hierarchy AS oh
  ON oh.o_custkey IN (
    SELECT
      c.c_custkey
    FROM customer AS c
    WHERE
      c.c_nationkey = n.n_nationkey
  )
LEFT JOIN lineitem_summary AS ls
  ON oh.o_orderkey = ls.l_orderkey
WHERE
  (
    ss.total_supply_value > 100000 OR ss.supplier_count > 5
  ) AND oh.depth <= 3
ORDER BY
  n.n_name,
  price_rank;
