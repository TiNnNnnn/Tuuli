WITH SalesSummary AS (
  SELECT
    ws_bill_customer_sk AS customer_id,
    SUM(ws_ext_sales_price) AS total_sales,
    COUNT(DISTINCT ws_order_number) AS order_count
  FROM web_sales
  WHERE
    ws_sold_date_sk BETWEEN 2500 AND 2550
  GROUP BY
    ws_bill_customer_sk
), CustomerDemographics AS (
  SELECT
    c.c_customer_sk AS customer_id,
    cd.cd_gender,
    cd.cd_marital_status,
    ib.ib_income_band_sk
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  JOIN household_demographics AS hd
    ON c.c_current_hdemo_sk = hd.hd_demo_sk
  JOIN income_band AS ib
    ON hd.hd_income_band_sk = ib.ib_income_band_sk
), AggregateData AS (
  SELECT
    cs.customer_id,
    cs.total_sales,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.ib_income_band_sk,
    ROW_NUMBER() OVER (PARTITION BY cd.ib_income_band_sk ORDER BY cs.total_sales DESC) AS rank
  FROM SalesSummary AS cs
  JOIN CustomerDemographics AS cd
    ON cs.customer_id = cd.customer_id
)
SELECT
  ad.customer_id,
  ad.total_sales,
  ad.cd_gender,
  ad.cd_marital_status,
  ad.ib_income_band_sk
FROM AggregateData AS ad
WHERE
  ad.rank <= 10
ORDER BY
  ad.ib_income_band_sk,
  ad.total_sales DESC;
