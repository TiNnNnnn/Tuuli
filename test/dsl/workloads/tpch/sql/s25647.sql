WITH processed_data AS (
  SELECT
    s.s_name AS supplier_name,
    p.p_name AS part_name,
    CONCAT(s.s_name, ' supplies ', p.p_name) AS supplier_part_info,
    p.p_retailprice,
    SUBSTRING(p.p_comment FROM 1 FOR 10) || '...' AS trimmed_comment,
    (
      CASE
        WHEN s.s_acctbal < 5000
        THEN 'Low Balance'
        WHEN s.s_acctbal BETWEEN 5000 AND 15000
        THEN 'Medium Balance'
        ELSE 'High Balance'
      END
    ) AS balance_category
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
)
SELECT
  balance_category,
  COUNT(*) AS supplier_count,
  AVG(p_retailprice) AS avg_retail_price,
  STRING_AGG(trimmed_comment, '; ') AS sample_comments,
  STRING_AGG(supplier_part_info, ', ') AS supplier_part_details
FROM processed_data
GROUP BY
  balance_category
ORDER BY
  supplier_count DESC;
