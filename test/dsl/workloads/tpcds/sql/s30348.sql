WITH RECURSIVE month_sales AS (
  SELECT
    d.d_year,
    d.d_month_seq,
    SUM(ws.ws_ext_sales_price) AS total_sales
  FROM date_dim AS d
  JOIN web_sales AS ws
    ON d.d_date_sk = ws.ws_sold_date_sk
  GROUP BY
    d.d_year,
    d.d_month_seq
  UNION ALL
  SELECT
    d.d_year,
    ms.d_month_seq + 1,
    SUM(ws.ws_ext_sales_price) AS total_sales
  FROM month_sales AS ms
  JOIN date_dim AS d
    ON ms.d_year = d.d_year AND ms.d_month_seq + 1 = d.d_month_seq
  JOIN web_sales AS ws
    ON d.d_date_sk = ws.ws_sold_date_sk
  WHERE
    ms.d_month_seq < 12
  GROUP BY
    d.d_year,
    ms.d_month_seq + 1
), customer_analysis AS (
  SELECT
    c.c_customer_id,
    cd.cd_gender,
    COUNT(DISTINCT ws.ws_order_number) AS order_count,
    SUM(ws.ws_net_profit) AS total_profit
  FROM customer AS c
  LEFT JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_id,
    cd.cd_gender
), top_customers AS (
  SELECT
    c.c_customer_id,
    ca.total_profit,
    ROW_NUMBER() OVER (ORDER BY ca.total_profit DESC) AS rank
  FROM customer_analysis AS ca
  JOIN customer AS c
    ON ca.c_customer_id = c.c_customer_id
  WHERE
    ca.total_profit > 1000
)
SELECT
  ms.d_year,
  ms.d_month_seq,
  SUM(ms.total_sales) AS monthly_sales,
  SUM(tc.total_profit) AS top_customer_profit
FROM month_sales AS ms
JOIN top_customers AS tc
  ON ms.d_year = 2023
GROUP BY
  ms.d_year,
  ms.d_month_seq
ORDER BY
  ms.d_month_seq;
