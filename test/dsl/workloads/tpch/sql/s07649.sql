WITH NationSupplier AS (
  SELECT
    n.n_name AS nation_name,
    COUNT(s.s_suppkey) AS supplier_count,
    SUM(s.s_acctbal) AS total_account_balance
  FROM nation AS n
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  GROUP BY
    n.n_name
), OrderSummary AS (
  SELECT
    c.c_nationkey,
    COUNT(o.o_orderkey) AS total_orders,
    SUM(o.o_totalprice) AS total_revenue
  FROM customer AS c
  JOIN orders AS o
    ON c.c_custkey = o.o_custkey
  GROUP BY
    c.c_nationkey
), PartAndSupplier AS (
  SELECT
    p.p_partkey,
    p.p_name,
    ps.ps_supplycost,
    ps.ps_availqty
  FROM part AS p
  JOIN partsupp AS ps
    ON p.p_partkey = ps.ps_partkey
  WHERE
    ps.ps_availqty > 0
), FinalReport AS (
  SELECT
    ns.nation_name,
    ns.supplier_count,
    ns.total_account_balance,
    os.total_orders,
    os.total_revenue,
    COUNT(pas.p_partkey) AS available_parts
  FROM NationSupplier AS ns
  LEFT JOIN OrderSummary AS os
    ON ns.nation_name = (
      SELECT
        n.n_name
      FROM nation AS n
      WHERE
        n.n_nationkey = os.c_nationkey
    )
  LEFT JOIN PartAndSupplier AS pas
    ON pas.ps_supplycost < (
      SELECT
        AVG(ps_supplycost)
      FROM partsupp
    )
  GROUP BY
    ns.nation_name,
    ns.supplier_count,
    ns.total_account_balance,
    os.total_orders,
    os.total_revenue
)
SELECT
  nation_name,
  supplier_count,
  total_account_balance,
  total_orders,
  total_revenue,
  available_parts
FROM FinalReport
ORDER BY
  total_revenue DESC,
  available_parts DESC;
