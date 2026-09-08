WITH RankedParts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_mfgr,
    p.p_brand,
    p.p_type,
    p.p_size,
    p.p_container,
    p.p_retailprice,
    p.p_comment,
    ROW_NUMBER() OVER (PARTITION BY p.p_brand ORDER BY p.p_retailprice DESC) AS rank
  FROM part AS p
  WHERE
    p.p_name LIKE '%widget%'
), SupplierInfo AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_address,
    s.s_phone,
    s.s_acctbal,
    s.s_comment,
    COUNT(ps.ps_partkey) AS part_count
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  GROUP BY
    s.s_suppkey,
    s.s_name,
    s.s_address,
    s.s_phone,
    s.s_acctbal,
    s.s_comment
), OrdersByCustomer AS (
  SELECT
    c.c_custkey,
    c.c_name,
    c.c_address,
    COUNT(o.o_orderkey) AS order_count
  FROM customer AS c
  LEFT JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_custkey,
    c.c_name,
    c.c_address
)
SELECT
  rp.p_name,
  rp.p_retailprice,
  si.s_name AS supplier_name,
  si.part_count,
  oc.c_name AS customer_name,
  oc.order_count
FROM RankedParts AS rp
JOIN SupplierInfo AS si
  ON rp.p_partkey IN (
    SELECT
      ps.ps_partkey
    FROM partsupp AS ps
    WHERE
      ps.ps_suppkey = si.s_suppkey
  )
JOIN OrdersByCustomer AS oc
  ON oc.order_count > 0
WHERE
  rp.rank <= 5
ORDER BY
  rp.p_retailprice DESC,
  si.part_count ASC;
