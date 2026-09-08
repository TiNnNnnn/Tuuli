WITH DetailedParts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_size,
    p.p_container,
    p.p_retailprice,
    p.p_comment,
    s.s_name AS supplier_name,
    c.c_name AS customer_name,
    r.r_name AS region_name
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  JOIN supplier AS s
    ON ps.ps_suppkey = s.s_suppkey
  JOIN orders AS o
    ON s.s_suppkey = o.o_custkey
  JOIN customer AS c
    ON o.o_custkey = c.c_custkey
  JOIN nation AS n
    ON s.s_nationkey = n.n_nationkey
  JOIN region AS r
    ON n.n_regionkey = r.r_regionkey
  WHERE
    p.p_retailprice > (
      SELECT
        AVG(p2.p_retailprice)
      FROM part AS p2
    )
    AND o.o_orderdate BETWEEN '1995-01-01' AND '1995-12-31'
)
SELECT
  p_partkey,
  p_name,
  COUNT(DISTINCT supplier_name) AS number_of_suppliers,
  COUNT(DISTINCT customer_name) AS number_of_customers,
  AVG(p_retailprice) AS average_retail_price,
  STRING_AGG(DISTINCT region_name, ', ') AS regions_supplied
FROM DetailedParts
GROUP BY
  p_partkey,
  p_name
ORDER BY
  average_retail_price DESC
LIMIT 10;
