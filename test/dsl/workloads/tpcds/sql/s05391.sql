WITH sales_summary AS (
  SELECT
    ws.ws_sold_date_sk,
    ws.ws_item_sk,
    SUM(ws.ws_quantity) AS total_quantity,
    SUM(ws.ws_net_profit) AS total_profit,
    SUM(ws.ws_ext_sales_price) AS total_sales
  FROM web_sales AS ws
  JOIN date_dim AS dd
    ON ws.ws_sold_date_sk = dd.d_date_sk
  WHERE
    dd.d_year = 2023 AND dd.d_month_seq BETWEEN 1 AND 6
  GROUP BY
    ws.ws_sold_date_sk,
    ws.ws_item_sk
), customer_summary AS (
  SELECT
    cd.cd_demo_sk,
    COUNT(DISTINCT c.c_customer_sk) AS total_customers,
    SUM(cd.cd_purchase_estimate) AS total_purchase_estimate
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  WHERE
    cd.cd_gender = 'M' AND cd.cd_marital_status = 'M'
  GROUP BY
    cd.cd_demo_sk
)
SELECT
  ss.ws_item_sk,
  ss.total_quantity,
  ss.total_profit,
  ss.total_sales,
  cs.total_customers,
  cs.total_purchase_estimate
FROM sales_summary AS ss
JOIN customer_summary AS cs
  ON cs.cd_demo_sk IN (
    SELECT DISTINCT
      i.i_item_sk
    FROM item AS i
    JOIN store_sales AS s
      ON i.i_item_sk = s.ss_item_sk
    WHERE
      s.ss_sold_date_sk BETWEEN 20230101 AND 20230630
  )
ORDER BY
  ss.total_profit DESC,
  ss.total_sales DESC
LIMIT 100;
