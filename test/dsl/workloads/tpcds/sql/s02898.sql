WITH SalesData AS (
  SELECT
    ws.ws_item_sk,
    ws.ws_sales_price,
    ws.ws_quantity,
    ws.ws_net_profit,
    d.d_year,
    d.d_month_seq,
    d.d_week_seq,
    ROW_NUMBER() OVER (PARTITION BY d.d_year, d.d_month_seq ORDER BY ws.ws_net_profit DESC) AS rank_profit
  FROM web_sales AS ws
  JOIN date_dim AS d
    ON ws.ws_sold_date_sk = d.d_date_sk
), CustomerDemo AS (
  SELECT
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    hd.hd_income_band_sk
  FROM customer_demographics AS cd
  LEFT JOIN household_demographics AS hd
    ON cd.cd_demo_sk = hd.hd_demo_sk
), AggregatedSales AS (
  SELECT
    cs.cs_item_sk,
    SUM(cs.cs_quantity) AS total_quantity,
    SUM(cs.cs_net_profit) AS total_profit
  FROM catalog_sales AS cs
  GROUP BY
    cs.cs_item_sk
)
SELECT
  ca.ca_address_id,
  c.c_first_name,
  c.c_last_name,
  cd.cd_gender,
  SUM(sd.ws_quantity) AS total_web_sales_quantity,
  AVG(sd.ws_sales_price) AS avg_web_sales_price,
  COALESCE(SUM(sd.ws_net_profit), 0) AS total_web_profit,
  r.r_reason_desc,
  CASE
    WHEN cd.cd_marital_status = 'M'
    THEN 'Married'
    WHEN cd.cd_marital_status IS NULL
    THEN 'Unknown'
    ELSE 'Single'
  END AS marital_status,
  CASE WHEN sd.rank_profit <= 10 THEN 'Top Profit' ELSE 'Regular Sales' END AS sale_category
FROM SalesData AS sd
LEFT JOIN customer AS c
  ON sd.ws_item_sk = c.c_customer_sk
LEFT JOIN customer_address AS ca
  ON c.c_current_addr_sk = ca.ca_address_sk
LEFT JOIN CustomerDemo AS cd
  ON c.c_current_cdemo_sk = cd.cd_demo_sk
LEFT JOIN reason AS r
  ON r.r_reason_sk = sd.ws_item_sk
LEFT JOIN AggregatedSales AS ag
  ON ag.cs_item_sk = sd.ws_item_sk
WHERE
  ca.ca_country = 'USA' AND (
    cd.cd_gender = 'F' OR cd.cd_gender IS NULL
  )
GROUP BY
  ca.ca_address_id,
  c.c_first_name,
  c.c_last_name,
  cd.cd_gender,
  r.r_reason_desc,
  cd.cd_marital_status,
  sd.rank_profit
HAVING
  SUM(sd.ws_quantity) > 100
ORDER BY
  total_web_profit DESC;
