WITH CustomerSales AS (
  SELECT
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    SUM(ws.ws_ext_sales_price) AS total_web_sales,
    COUNT(ws.ws_order_number) AS order_count
  FROM customer AS c
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name
), HighValueCustomers AS (
  SELECT
    cs.c_customer_sk,
    cs.c_first_name,
    cs.c_last_name,
    cs.total_web_sales,
    cs.order_count,
    DENSE_RANK() OVER (ORDER BY cs.total_web_sales DESC) AS sales_rank
  FROM CustomerSales AS cs
  WHERE
    NOT cs.total_web_sales IS NULL
), DemographicsByIncome AS (
  SELECT
    cd.cd_demo_sk,
    cd.cd_gender,
    ib.ib_income_band_sk,
    COUNT(DISTINCT c.c_customer_sk) AS demographic_count
  FROM customer_demographics AS cd
  JOIN household_demographics AS hd
    ON cd.cd_demo_sk = hd.hd_demo_sk
  LEFT JOIN income_band AS ib
    ON hd.hd_income_band_sk = ib.ib_income_band_sk
  JOIN customer AS c
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  GROUP BY
    cd.cd_demo_sk,
    cd.cd_gender,
    ib.ib_income_band_sk
)
SELECT
  hvc.c_first_name,
  hvc.c_last_name,
  hvc.total_web_sales,
  hvc.order_count,
  dbi.ib_income_band_sk,
  dbi.demographic_count,
  CASE WHEN hvc.order_count > 10 THEN 'Frequent' ELSE 'Occasional' END AS customer_type
FROM HighValueCustomers AS hvc
LEFT JOIN DemographicsByIncome AS dbi
  ON hvc.c_customer_sk = dbi.cd_demo_sk
WHERE
  hvc.sales_rank <= 10
ORDER BY
  hvc.total_web_sales DESC;
