WITH RankedSales AS (
  SELECT
    s.s_store_id,
    SUM(ss_ext_sales_price) AS total_sales,
    COUNT(DISTINCT ss_ticket_number) AS total_transactions,
    RANK() OVER (PARTITION BY s.s_store_id ORDER BY SUM(ss_ext_sales_price) DESC) AS sales_rank
  FROM store_sales
  JOIN store AS s
    ON s.s_store_sk = ss_store_sk
  GROUP BY
    s.s_store_id
), CustomerPurchaseStats AS (
  SELECT
    c.c_customer_id,
    COALESCE(SUM(ws_ext_sales_price), 0) AS total_web_sales,
    COALESCE(SUM(ss_ext_sales_price), 0) AS total_store_sales
  FROM customer AS c
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  LEFT JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  GROUP BY
    c.c_customer_id
), TargetIncomeBand AS (
  SELECT
    ib.ib_income_band_sk,
    (
      ib.ib_lower_bound + ib.ib_upper_bound
    ) / 2 AS average_income
  FROM income_band AS ib
  WHERE
    NOT ib.ib_lower_bound IS NULL AND NOT ib.ib_upper_bound IS NULL
)
SELECT
  a.ca_city,
  COUNT(DISTINCT c.c_customer_id) AS customer_count,
  SUM(cp.total_web_sales) AS total_web_sales,
  SUM(cp.total_store_sales) AS total_store_sales,
  MAX(rs.total_sales) AS highest_store_sales,
  t.average_income
FROM customer_address AS a
JOIN customer AS c
  ON a.ca_address_sk = c.c_current_addr_sk
LEFT JOIN CustomerPurchaseStats AS cp
  ON c.c_customer_id = cp.c_customer_id
LEFT JOIN RankedSales AS rs
  ON rs.s_store_id IN (
    SELECT
      s_store_id
    FROM store
    WHERE
      s_city = a.ca_city
  )
LEFT JOIN TargetIncomeBand AS t
  ON c.c_current_cdemo_sk = t.ib_income_band_sk
GROUP BY
  a.ca_city,
  t.average_income
HAVING
  COUNT(DISTINCT c.c_customer_id) > 10
ORDER BY
  customer_count DESC;
