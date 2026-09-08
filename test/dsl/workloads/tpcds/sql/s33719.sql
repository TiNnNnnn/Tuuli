WITH RECURSIVE monthly_sales AS (
  SELECT
    d.d_year AS year,
    d.d_month_seq AS month,
    SUM(ss_ext_sales_price) AS total_sales
  FROM date_dim AS d
  JOIN store_sales AS s
    ON d.d_date_sk = s.ss_sold_date_sk
  GROUP BY
    d.d_year,
    d.d_month_seq
  UNION ALL
  SELECT
    year,
    month + 1,
    total_sales
  FROM monthly_sales
  WHERE
    month < 12
), customer_address_info AS (
  SELECT
    ca.ca_address_id,
    ca.ca_city,
    COALESCE(SUM(CASE WHEN NOT s.ss_item_sk IS NULL THEN s.ss_net_paid END), 0) AS total_spent
  FROM customer_address AS ca
  LEFT JOIN customer AS c
    ON ca.ca_address_sk = c.c_current_addr_sk
  LEFT JOIN store_sales AS s
    ON c.c_customer_sk = s.ss_customer_sk
  GROUP BY
    ca.ca_address_id,
    ca.ca_city
), top_customers AS (
  SELECT
    ca.ca_city,
    ca.total_spent,
    RANK() OVER (PARTITION BY ca.ca_city ORDER BY ca.total_spent DESC) AS city_rank
  FROM customer_address_info AS ca
)
SELECT
  mc.year,
  mc.month,
  tc.ca_city,
  tc.total_spent
FROM monthly_sales AS mc
JOIN top_customers AS tc
  ON mc.month = tc.city_rank
WHERE
  mc.total_sales > (
    SELECT
      AVG(total_sales)
    FROM monthly_sales
  )
ORDER BY
  mc.year,
  mc.month,
  tc.total_spent DESC;
