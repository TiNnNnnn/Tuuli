WITH CustomerSales AS (
  SELECT
    c.c_customer_sk,
    SUM(ss.ss_ext_sales_price) AS total_sales,
    COUNT(DISTINCT ss.ss_ticket_number) AS total_transactions,
    AVG(ss.ss_ext_sales_price) AS average_transaction_value
  FROM customer AS c
  JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  WHERE
    ss.ss_sold_date_sk BETWEEN 2458849 AND 2458915
  GROUP BY
    c.c_customer_sk
), DemographicAnalysis AS (
  SELECT
    cd.cd_demo_sk,
    MAX(cd.cd_credit_rating) AS max_credit_rating,
    MIN(cd.cd_purchase_estimate) AS min_purchase_estimate,
    AVG(cd.cd_dep_count) AS average_dependents
  FROM customer_demographics AS cd
  JOIN CustomerSales AS cs
    ON cs.c_customer_sk = cd.cd_demo_sk
  GROUP BY
    cd.cd_demo_sk
), SalesSummary AS (
  SELECT
    cs.c_customer_sk,
    ds.max_credit_rating,
    ds.min_purchase_estimate,
    ds.average_dependents,
    cs.total_sales,
    cs.total_transactions,
    cs.average_transaction_value
  FROM CustomerSales AS cs
  JOIN DemographicAnalysis AS ds
    ON cs.c_customer_sk = ds.cd_demo_sk
)
SELECT
  s.c_customer_sk,
  s.total_sales,
  s.total_transactions,
  s.average_transaction_value,
  CASE
    WHEN s.total_sales > 1000
    THEN 'High Value Customer'
    WHEN s.total_sales BETWEEN 500 AND 1000
    THEN 'Mid Value Customer'
    ELSE 'Low Value Customer'
  END AS customer_value_category
FROM SalesSummary AS s
ORDER BY
  s.total_sales DESC
LIMIT 100;
