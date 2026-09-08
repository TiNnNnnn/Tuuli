WITH CustomerInfo AS (
  SELECT
    c.c_customer_id,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_education_status,
    SUM(ss.ss_sales_price) AS total_sales
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  GROUP BY
    c.c_customer_id,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_education_status
), TopCustomers AS (
  SELECT
    c.*,
    RANK() OVER (PARTITION BY c.cd_gender ORDER BY c.total_sales DESC) AS sales_rank
  FROM CustomerInfo AS c
)
SELECT
  tc.c_customer_id,
  tc.cd_gender,
  tc.cd_marital_status,
  tc.cd_education_status,
  tc.total_sales
FROM TopCustomers AS tc
WHERE
  tc.sales_rank <= 10
ORDER BY
  tc.cd_gender,
  tc.total_sales DESC;
