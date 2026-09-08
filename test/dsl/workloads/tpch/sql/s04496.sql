WITH RegionalSales AS (
  SELECT
    n.n_name AS nation_name,
    r.r_name AS region_name,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM nation AS n
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  GROUP BY
    n.n_name,
    r.r_name
), CustomerOrders AS (
  SELECT
    c.c_custkey,
    SUM(o.o_totalprice) AS customer_spending
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey
), TopCustomers AS (
  SELECT
    c.c_custkey,
    c.c_name,
    COALESCE(co.customer_spending, 0) AS total_spending
  FROM customer AS c
  LEFT JOIN CustomerOrders AS co
    ON c.c_custkey = co.c_custkey
  ORDER BY
    total_spending DESC
  LIMIT 10
)
SELECT
  r.nation_name,
  r.region_name,
  r.total_sales,
  tc.c_custkey,
  tc.c_name,
  tc.total_spending
FROM RegionalSales AS r
LEFT JOIN TopCustomers AS tc
  ON r.nation_name = (
    SELECT
      n.n_name
    FROM nation AS n
    JOIN supplier AS s
      ON n.n_nationkey = s.s_nationkey
    WHERE
      s.s_suppkey = (
        SELECT
          ps.ps_suppkey
        FROM partsupp AS ps
        JOIN part AS p
          ON ps.ps_partkey = p.p_partkey
        WHERE
          p.p_name LIKE '%metal%'
        LIMIT 1
      )
  )
WHERE
  r.total_sales > (
    SELECT
      AVG(total_sales)
    FROM RegionalSales
  )
ORDER BY
  r.total_sales DESC,
  tc.total_spending DESC;
