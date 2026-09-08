WITH CustomerReturns AS (
  SELECT
    c.c_customer_id,
    SUM(sr_return_quantity) AS total_returned_quantity,
    SUM(sr_return_amt) AS total_returned_amt
  FROM customer AS c
  JOIN store_returns AS sr
    ON c.c_customer_sk = sr.sr_customer_sk
  GROUP BY
    c.c_customer_id
), SalesData AS (
  SELECT
    c.c_customer_id,
    COUNT(DISTINCT ws.ws_order_number) AS total_orders,
    SUM(ws.ws_sales_price) AS total_sales_amt
  FROM customer AS c
  JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_id
), Demographics AS (
  SELECT
    cd.cd_gender,
    cd.cd_marital_status,
    SUM(COALESCE(cr.total_returned_quantity, 0)) AS total_returned_quantity,
    SUM(sd.total_orders) AS total_orders,
    SUM(sd.total_sales_amt) AS total_sales_amt
  FROM customer_demographics AS cd
  LEFT JOIN CustomerReturns AS cr
    ON cd.cd_demo_sk = (
      SELECT
        c.c_current_cdemo_sk
      FROM customer AS c
      WHERE
        c.c_customer_id = cr.c_customer_id
    )
  LEFT JOIN SalesData AS sd
    ON cd.cd_demo_sk = (
      SELECT
        c.c_current_cdemo_sk
      FROM customer AS c
      WHERE
        c.c_customer_id = sd.c_customer_id
    )
  GROUP BY
    cd.cd_gender,
    cd.cd_marital_status
)
SELECT
  cd_gender,
  cd_marital_status,
  total_orders,
  total_returned_quantity,
  total_sales_amt,
  total_sales_amt - total_returned_quantity AS net_sales
FROM Demographics
ORDER BY
  cd_gender,
  cd_marital_status;
