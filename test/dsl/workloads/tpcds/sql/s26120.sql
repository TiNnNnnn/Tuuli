WITH customer_full_names AS (
  SELECT
    c.c_customer_sk,
    CONCAT(c.c_first_name, ' ', c.c_last_name) AS full_name,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_education_status,
    cd.cd_purchase_estimate
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
), item_details AS (
  SELECT
    i.i_item_sk,
    i.i_item_desc,
    i.i_current_price,
    i.i_brand,
    i.i_category
  FROM item AS i
), sales_summary AS (
  SELECT
    ws.ws_item_sk,
    SUM(ws.ws_quantity) AS total_quantity_sold,
    SUM(ws.ws_ext_sales_price) AS total_sales_amount,
    COUNT(DISTINCT ws.ws_order_number) AS total_orders
  FROM web_sales AS ws
  GROUP BY
    ws.ws_item_sk
)
SELECT
  cf.full_name,
  cf.cd_gender,
  cf.cd_marital_status,
  cf.cd_education_status,
  id.i_item_desc,
  id.i_brand,
  ss.total_quantity_sold,
  ss.total_sales_amount,
  ss.total_orders
FROM customer_full_names AS cf
JOIN sales_summary AS ss
  ON cf.c_customer_sk = (
    SELECT
      cs.ss_customer_sk
    FROM store_sales AS cs
    WHERE
      cs.ss_item_sk IN (
        SELECT
          ws.ws_item_sk
        FROM sales_summary AS ws
      )
    LIMIT 1
  )
JOIN item_details AS id
  ON ss.ws_item_sk = id.i_item_sk
WHERE
  cf.cd_gender = 'F' AND cf.cd_marital_status = 'M' AND ss.total_quantity_sold > 100
ORDER BY
  ss.total_sales_amount DESC
LIMIT 50;
