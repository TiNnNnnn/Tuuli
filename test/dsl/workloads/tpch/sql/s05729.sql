WITH RegionalSales AS (
  SELECT
    n.n_name AS nation,
    SUM(l.l_extendedprice * (
      1 - l.l_discount
    )) AS total_sales
  FROM nation AS n
  JOIN supplier AS s
    ON n.n_nationkey = s.s_nationkey
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
  JOIN lineitem AS l
    ON p.p_partkey = l.l_partkey
  GROUP BY
    n.n_name
), SalesRanked AS (
  SELECT
    nation,
    total_sales,
    RANK() OVER (ORDER BY total_sales DESC) AS sales_rank
  FROM RegionalSales
)
SELECT
  sr.nation,
  sr.total_sales,
  sr.sales_rank,
  r.r_comment
FROM SalesRanked AS sr
JOIN region AS r
  ON (
    sr.nation = (
      SELECT
        n.n_name
      FROM nation AS n
      WHERE
        n.n_nationkey = r.r_regionkey
    )
  )
WHERE
  sr.sales_rank <= 10
ORDER BY
  sr.sales_rank;
