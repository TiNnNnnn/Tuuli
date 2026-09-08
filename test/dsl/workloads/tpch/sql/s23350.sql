WITH ranked_parts AS (
  SELECT
    p.p_partkey,
    p.p_name,
    p.p_retailprice,
    ROW_NUMBER() OVER (PARTITION BY p.p_brand ORDER BY p.p_retailprice DESC) AS rn
  FROM part AS p
  WHERE
    p.p_size IN (
      SELECT DISTINCT
        p2.p_size
      FROM part AS p2
      WHERE
        p2.p_retailprice > 100 AND p2.p_type LIKE '%gold%'
    )
), supplier_info AS (
  SELECT
    s.s_suppkey,
    s.s_name,
    s.s_acctbal,
    (
      CASE WHEN s.s_acctbal IS NULL THEN 'Unknown' ELSE 'Known' END
    ) AS account_status
  FROM supplier AS s
  WHERE
    s.s_nationkey IN (
      SELECT
        n.n_nationkey
      FROM nation AS n
      WHERE
        NOT n.n_comment IS NULL
    )
), order_summary AS (
  SELECT
    o.o_orderkey,
    o.o_orderdate,
    SUM(li.l_extendedprice * (
      1 - li.l_discount
    )) AS total_revenue,
    COUNT(DISTINCT li.l_linenumber) AS item_count,
    MAX(li.l_shipdate) AS last_ship_date
  FROM orders AS o
  JOIN lineitem AS li
    ON o.o_orderkey = li.l_orderkey
  WHERE
    o.o_orderstatus IN ('O', 'F')
  GROUP BY
    o.o_orderkey,
    o.o_orderdate
)
SELECT
  r.p_partkey,
  r.p_name,
  r.p_retailprice,
  COALESCE(s.s_suppkey, -1) AS supplier_key,
  s.account_status,
  o.o_orderkey,
  o.total_revenue,
  CASE
    WHEN o.last_ship_date < CAST('1998-10-01' AS DATE) - INTERVAL '30 DAYS'
    THEN 'Old Shipment'
    ELSE 'Recent Shipment'
  END AS shipment_status
FROM ranked_parts AS r
LEFT JOIN partsupp AS ps
  ON r.p_partkey = ps.ps_partkey
LEFT JOIN supplier_info AS s
  ON ps.ps_suppkey = s.s_suppkey
LEFT JOIN order_summary AS o
  ON r.p_partkey = o.o_orderkey
WHERE
  r.rn <= 5 AND (
    NOT s.s_acctbal IS NULL OR s.s_acctbal IS NULL
  )
ORDER BY
  r.p_retailprice ASC,
  o.total_revenue DESC
FETCH FIRST 100 ROWS ONLY;
