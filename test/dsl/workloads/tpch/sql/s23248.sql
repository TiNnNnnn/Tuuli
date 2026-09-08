WITH RankedOrders AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    o.o_totalprice,
    o.o_orderstatus,
    ROW_NUMBER() OVER (PARTITION BY o.o_orderstatus ORDER BY o.o_totalprice DESC) AS rn_status,
    DENSE_RANK() OVER (ORDER BY o.o_totalprice) AS dr_totalprice
  FROM orders AS o
  WHERE
    o.o_orderdate >= CURRENT_DATE - INTERVAL '1 YEAR'
), SupplierDetails AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    SUM(ps.ps_supplycost * ps.ps_availqty) AS total_supplycost,
    STRING_AGG(s.s_comment, ', ') AS comments
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name
  HAVING
    NOT SUM(ps.ps_supplycost * ps.ps_availqty) IS NULL
), LargestPart AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_retailprice,
    COUNT(DISTINCT ps.ps_suppkey) AS supplier_count
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  GROUP BY
    p.p_partkey,
    p.p_name,
    p.p_retailprice
  HAVING
    p.p_retailprice > (
      SELECT
        AVG(p2.p_retailprice)
      FROM part AS p2
    )
  ORDER BY
    supplier_count DESC
  LIMIT 1
)
SELECT
  r.o_orderkey,
  r.o_orderdate,
  r.o_totalprice,
  d.s_name AS supplier_name,
  p.p_name AS part_name,
  d.total_supplycost,
  d.comments,
  CASE
    WHEN r.o_orderstatus = 'O'
    THEN 'Active'
    WHEN r.o_orderstatus = 'F'
    THEN 'Finalized'
    ELSE 'Unknown'
  END AS order_status_description
FROM RankedOrders AS r
LEFT JOIN SupplierDetails AS d
  ON r.o_orderkey = d.s_suppkey
INNER JOIN LargestPart AS p
  ON p.p_partkey = (
    SELECT
      ps.ps_partkey
    FROM partsupp AS ps
    WHERE
      ps.ps_suppkey = d.s_suppkey
    ORDER BY
      ps.ps_availqty DESC
    LIMIT 1
  )
WHERE
  r.rn_status <= 10
  AND (
    d.total_supplycost IS NULL OR d.total_supplycost >= 1000
  )
  AND NOT r.o_orderkey IS NULL
  AND COALESCE(r.o_orderdate, CAST('1900-01-01' AS DATE)) > CAST('2000-01-01' AS DATE)
ORDER BY
  r.o_totalprice DESC,
  d.comments
OFFSET 5
FETCH NEXT 10 ROWS ONLY;
