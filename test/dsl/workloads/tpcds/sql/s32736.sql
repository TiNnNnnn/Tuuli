WITH RECURSIVE CustomerCTE AS (
  SELECT
    c_customer_sk,
    c_first_name,
    c_last_name,
    1 AS level
  FROM customer
  WHERE
    c_customer_sk IN (
      SELECT DISTINCT
        sr_customer_sk
      FROM store_returns
    )
  UNION ALL
  SELECT
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    cte.level + 1
  FROM customer AS c
  JOIN CustomerCTE AS cte
    ON cte.c_customer_sk = c.c_current_cdemo_sk
), SalesData AS (
  SELECT
    ws_bill_customer_sk,
    SUM(ws_net_paid) AS total_spent,
    COUNT(ws_order_number) AS order_count,
    ROW_NUMBER() OVER (PARTITION BY ws_bill_customer_sk ORDER BY SUM(ws_net_paid) DESC) AS sales_rank
  FROM web_sales
  GROUP BY
    ws_bill_customer_sk
), ReturnedData AS (
  SELECT
    sr_customer_sk,
    SUM(sr_return_amt) AS total_returns,
    COUNT(sr_ticket_number) AS return_count
  FROM store_returns
  GROUP BY
    sr_customer_sk
), FinalData AS (
  SELECT
    ccte.c_first_name,
    ccte.c_last_name,
    sd.total_spent,
    rd.total_returns,
    COALESCE(sd.order_count, 0) AS order_count,
    COALESCE(rd.return_count, 0) AS return_count,
    (
      COALESCE(sd.total_spent, 0) - COALESCE(rd.total_returns, 0)
    ) AS net_spent
  FROM CustomerCTE AS ccte
  LEFT JOIN SalesData AS sd
    ON ccte.c_customer_sk = sd.ws_bill_customer_sk
  LEFT JOIN ReturnedData AS rd
    ON ccte.c_customer_sk = rd.sr_customer_sk
)
SELECT
  f.c_first_name,
  f.c_last_name,
  f.total_spent,
  f.total_returns,
  f.order_count,
  f.return_count,
  f.net_spent,
  CASE
    WHEN f.net_spent > 500
    THEN 'High Value'
    WHEN f.net_spent BETWEEN 200 AND 500
    THEN 'Medium Value'
    ELSE 'Low Value'
  END AS customer_value
FROM FinalData AS f
WHERE
  NOT f.net_spent IS NULL
ORDER BY
  f.net_spent DESC
LIMIT 100;
