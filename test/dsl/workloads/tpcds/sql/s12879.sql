WITH TotalSales AS (
  SELECT
    c.c_customer_sk,
    SUM(ws.ws_ext_sales_price) AS total_sales
  FROM customer AS c
  JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_sk
), TopCustomers AS (
  SELECT
    c.c_customer_id,
    ts.total_sales
  FROM TotalSales AS ts
  JOIN customer AS c
    ON ts.c_customer_sk = c.c_customer_sk
  ORDER BY
    ts.total_sales DESC
  LIMIT 10
)
SELECT
  tc.c_customer_id,
  tc.total_sales
FROM TopCustomers AS tc;
