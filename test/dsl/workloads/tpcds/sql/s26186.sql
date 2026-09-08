WITH CustomerSales AS (
  SELECT
    c.c_customer_id,
    CONCAT(c.c_first_name, ' ', c.c_last_name) AS full_name,
    SUM(ws.ws_ext_sales_price) AS total_sales,
    COUNT(DISTINCT ws.ws_order_number) AS order_count
  FROM customer AS c
  JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_id,
    c.c_first_name,
    c.c_last_name
), TopCustomers AS (
  SELECT
    cs.full_name,
    cs.total_sales,
    cs.order_count,
    RANK() OVER (ORDER BY cs.total_sales DESC) AS sales_rank
  FROM CustomerSales AS cs
), SalesDetails AS (
  SELECT
    tc.full_name,
    tc.total_sales,
    tc.order_count,
    d.d_date AS sale_date,
    i.i_item_desc,
    ws.ws_ext_sales_price,
    ws.ws_order_number
  FROM TopCustomers AS tc
  JOIN web_sales AS ws
    ON tc.full_name = CONCAT(
      (
        SELECT
          c.c_first_name
        FROM customer AS c
        WHERE
          c.c_customer_id = tc.full_name
        LIMIT 1
      ),
      ' ',
      (
        SELECT
          c.c_last_name
        FROM customer AS c
        WHERE
          c.c_customer_id = tc.full_name
        LIMIT 1
      )
    )
  JOIN item AS i
    ON ws.ws_item_sk = i.i_item_sk
  JOIN date_dim AS d
    ON ws.ws_sold_date_sk = d.d_date_sk
  WHERE
    tc.sales_rank <= 10
)
SELECT
  sd.full_name,
  sd.sale_date,
  sd.i_item_desc,
  sd.ws_ext_sales_price,
  CONCAT(SUBSTRING(sd.full_name FROM 1 FOR 5), '...') AS abbreviated_name
FROM SalesDetails AS sd
ORDER BY
  sd.sale_date DESC,
  sd.ws_ext_sales_price DESC;
