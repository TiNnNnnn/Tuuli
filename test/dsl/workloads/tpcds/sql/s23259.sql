WITH ranked_sales AS (
  SELECT
    cs_bill_customer_sk,
    SUM(cs_ext_sales_price) AS total_sales,
    RANK() OVER (PARTITION BY cs_bill_customer_sk ORDER BY SUM(cs_ext_sales_price) DESC) AS sales_rank
  FROM catalog_sales
  WHERE
    cs_sold_date_sk BETWEEN (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_year = 2023
    ) - 30 AND (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_year = 2023
    )
  GROUP BY
    cs_bill_customer_sk
), customer_info AS (
  SELECT
    c.c_customer_id,
    ca.ca_city,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_credit_rating,
    ci.total_sales,
    ci.sales_rank
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  LEFT JOIN customer_address AS ca
    ON c.c_current_addr_sk = ca.ca_address_sk
  LEFT JOIN ranked_sales AS ci
    ON c.c_customer_sk = ci.cs_bill_customer_sk
), sales_summary AS (
  SELECT
    COALESCE(ci.c_customer_id, 'Unknown') AS customer_id,
    COUNT(ci.sales_rank) AS number_of_purchases,
    AVG(ci.total_sales) AS average_purchase_value,
    MAX(CASE WHEN ci.cd_gender = 'F' THEN ci.total_sales END) AS max_female_sales,
    MIN(CASE WHEN ci.cd_gender = 'M' THEN ci.total_sales END) AS min_male_sales
  FROM customer_info AS ci
  GROUP BY
    ci.c_customer_id
  HAVING
    COUNT(ci.sales_rank) > 0
)
SELECT
  ss.customer_id,
  ss.number_of_purchases,
  ss.average_purchase_value,
  ss.max_female_sales,
  ss.min_male_sales,
  CASE
    WHEN ss.average_purchase_value IS NULL
    THEN 'No Sales'
    WHEN ss.average_purchase_value > 1000
    THEN 'High Value Customer'
    ELSE 'Regular Customer'
  END AS customer_category,
  STRING_AGG(
    CASE
      WHEN NOT ss.max_female_sales IS NULL AND NOT ss.min_male_sales IS NULL
      THEN 'Mixed Sales'
      ELSE 'Single Gender Sales'
    END,
    ','
  ) AS sales_type
FROM sales_summary AS ss
GROUP BY
  ss.customer_id,
  ss.number_of_purchases,
  ss.average_purchase_value,
  ss.max_female_sales,
  ss.min_male_sales
ORDER BY
  ss.average_purchase_value DESC
LIMIT 100;
