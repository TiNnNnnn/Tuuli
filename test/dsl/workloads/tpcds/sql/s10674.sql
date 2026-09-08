SELECT
  c.c_customer_id,
  ca.ca_city,
  d.d_year,
  SUM(ws.ws_sales_price) AS total_sales
FROM web_sales AS ws
JOIN customer AS c
  ON ws.ws_bill_customer_sk = c.c_customer_sk
JOIN customer_address AS ca
  ON c.c_current_addr_sk = ca.ca_address_sk
JOIN date_dim AS d
  ON ws.ws_sold_date_sk = d.d_date_sk
WHERE
  d.d_year = 2023
GROUP BY
  c.c_customer_id,
  ca.ca_city,
  d.d_year
ORDER BY
  total_sales DESC
LIMIT 10;
