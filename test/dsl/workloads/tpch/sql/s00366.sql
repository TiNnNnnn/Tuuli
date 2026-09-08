WITH RankedCustomers AS (
  SELECT
    c.c_custkey,
    c.c_name,
    SUM(o.o_totalprice) AS total_spent,
    ROW_NUMBER() OVER (PARTITION BY c.c_nationkey ORDER BY SUM(o.o_totalprice) DESC) AS rank
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name,
    c.c_nationkey
), TopSuppliers AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_availqty * ps.ps_supplycost) AS total_supply_value
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
  HAVING
    SUM(ps.ps_availqty * ps.ps_supplycost) > 10000
), FilteredLineItems AS (
  SELECT
    l.l_orderkey,
    l.l_partkey,
    l.l_suppkey,
    l.l_quantity,
    l.l_discount,
    l.l_extendedprice * (
      1 - l.l_discount
    ) AS net_price,
    CASE WHEN l.l_returnflag = 'R' THEN 'Returned' ELSE 'Not Returned' END AS return_status
  FROM lineitem AS l
  WHERE
    l.l_shipdate >= CAST('1997-01-01' AS DATE)
    AND l.l_shipdate < CAST('1997-12-31' AS DATE)
)
SELECT
  rc.c_name AS customer_name,
  rc.total_spent AS total_spent,
  ts.s_name AS supplier_name,
  fli.l_orderkey AS order_key,
  SUM(fli.net_price) AS total_net_price,
  COUNT(DISTINCT fli.l_partkey) AS unique_parts,
  MAX(fli.return_status) AS max_return_status
FROM RankedCustomers AS rc
LEFT JOIN FilteredLineItems AS fli
  ON fli.l_orderkey IN (
    SELECT
      o.o_orderkey
    FROM orders AS o
    WHERE
      o.o_custkey = rc.c_custkey
  )
LEFT JOIN TopSuppliers AS ts
  ON fli.l_suppkey = ts.s_suppkey
WHERE
  rc.rank <= 5
GROUP BY
  rc.c_name,
  rc.total_spent,
  ts.s_name,
  fli.l_orderkey
ORDER BY
  rc.total_spent DESC,
  unique_parts DESC;
