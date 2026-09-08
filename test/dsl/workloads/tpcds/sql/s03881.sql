WITH CustomerSales AS (
  SELECT
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    SUM(ss.ss_net_paid) AS total_sales,
    COUNT(ss.ss_ticket_number) AS total_transactions
  FROM customer AS c
  JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  GROUP BY
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name
), SalesRank AS (
  SELECT
    cs.c_customer_sk,
    cs.c_first_name,
    cs.c_last_name,
    cs.total_sales,
    cs.total_transactions,
    RANK() OVER (ORDER BY cs.total_sales DESC) AS sales_rank
  FROM CustomerSales AS cs
  WHERE
    cs.total_sales > 0
), TopCustomers AS (
  SELECT
    tr.c_customer_sk,
    tr.c_first_name,
    tr.c_last_name,
    tr.total_sales,
    tr.total_transactions,
    tr.sales_rank,
    (
      SELECT
        AVG(total_sales)
      FROM CustomerSales
    ) AS avg_sales
  FROM SalesRank AS tr
  WHERE
    tr.sales_rank <= 10
)
SELECT
  tc.c_customer_sk,
  tc.c_first_name,
  tc.c_last_name,
  tc.total_sales,
  tc.total_transactions,
  CASE
    WHEN tc.total_sales > tc.avg_sales
    THEN 'Above Average'
    ELSE 'Below Average'
  END AS sales_performance,
  COALESCE(
    (
      SELECT
        SUM(ws.ws_ext_sales_price)
      FROM web_sales AS ws
      WHERE
        ws.ws_bill_customer_sk = tc.c_customer_sk
    ),
    0
  ) AS total_web_sales,
  ROW_NUMBER() OVER (ORDER BY tc.total_sales DESC) AS row_num
FROM TopCustomers AS tc
LEFT JOIN customer_address AS ca
  ON ca.ca_address_sk = (
    SELECT
      c.c_current_addr_sk
    FROM customer AS c
    WHERE
      c.c_customer_sk = tc.c_customer_sk
  )
WHERE
  ca.ca_state = 'CA'
ORDER BY
  tc.total_sales DESC,
  tc.total_transactions ASC;
