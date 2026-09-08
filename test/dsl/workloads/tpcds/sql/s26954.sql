WITH AddressDetails AS (
  SELECT
    ca_address_sk,
    CONCAT(
      ca_street_number,
      ' ',
      ca_street_name,
      ' ',
      ca_street_type,
      CASE
        WHEN NOT ca_suite_number IS NULL
        THEN CONCAT(' Suite ', ca_suite_number)
        ELSE ''
      END
    ) AS full_address,
    ca_city,
    ca_state,
    ca_zip
  FROM customer_address
), CustomerInfo AS (
  SELECT
    c.c_customer_sk,
    CONCAT(c.c_first_name, ' ', c.c_last_name) AS full_name,
    d.cd_gender,
    d.cd_marital_status,
    d.cd_education_status,
    CONCAT(c.c_birth_day, '-', c.c_birth_month, '-', c.c_birth_year) AS birth_date,
    ca.full_address
  FROM customer AS c
  JOIN customer_demographics AS d
    ON c.c_current_cdemo_sk = d.cd_demo_sk
  JOIN AddressDetails AS ca
    ON c.c_current_addr_sk = ca.ca_address_sk
), SalesData AS (
  SELECT
    ws.ws_item_sk,
    SUM(ws.ws_quantity) AS total_quantity,
    SUM(ws.ws_ext_sales_price) AS total_sales
  FROM web_sales AS ws
  GROUP BY
    ws.ws_item_sk
), TopItems AS (
  SELECT
    sd.ws_item_sk,
    sd.total_quantity,
    sd.total_sales,
    RANK() OVER (ORDER BY sd.total_sales DESC) AS sales_rank
  FROM SalesData AS sd
)
SELECT
  ci.full_name,
  ci.cd_gender,
  ci.cd_marital_status,
  ti.total_quantity,
  ti.total_sales,
  ti.sales_rank
FROM CustomerInfo AS ci
JOIN TopItems AS ti
  ON ci.c_customer_sk = (
    SELECT
      c.c_customer_sk
    FROM web_sales AS ws
    JOIN customer AS c
      ON ws.ws_bill_customer_sk = c.c_customer_sk
    WHERE
      ws.ws_item_sk = ti.ws_item_sk
    ORDER BY
      ws.ws_sold_date_sk DESC
    LIMIT 1
  )
WHERE
  ti.sales_rank <= 10
ORDER BY
  ti.total_sales DESC;
