WITH SalesData AS (
  SELECT
    ws.ws_order_number,
    ws.ws_item_sk,
    ws.ws_sales_price,
    ws.ws_quantity,
    i.i_item_desc,
    c.c_first_name,
    c.c_last_name,
    ca.ca_state,
    DENSE_RANK() OVER (PARTITION BY ca.ca_state ORDER BY SUM(ws.ws_sales_price)) AS state_sales_rank
  FROM web_sales AS ws
  JOIN item AS i
    ON ws.ws_item_sk = i.i_item_sk
  JOIN customer AS c
    ON ws.ws_bill_customer_sk = c.c_customer_sk
  JOIN customer_address AS ca
    ON c.c_current_addr_sk = ca.ca_address_sk
  WHERE
    ws.ws_sold_date_sk > 2458357
  GROUP BY
    ws.ws_order_number,
    ws.ws_item_sk,
    ws.ws_sales_price,
    ws.ws_quantity,
    i.i_item_desc,
    c.c_first_name,
    c.c_last_name,
    ca.ca_state
), AggregatedSales AS (
  SELECT
    sd.ca_state,
    COUNT(DISTINCT sd.ws_order_number) AS order_count,
    SUM(sd.ws_sales_price * sd.ws_quantity) AS total_sales,
    AVG(sd.ws_sales_price) AS avg_sales_price,
    SUM(sd.ws_quantity) AS total_quantity
  FROM SalesData AS sd
  WHERE
    sd.state_sales_rank <= 3
  GROUP BY
    sd.ca_state
)
SELECT
  ASH.ca_state,
  ASH.order_count,
  ASH.total_sales,
  ASH.avg_sales_price,
  ASH.total_quantity,
  COALESCE(IB.ib_lower_bound, 0) AS lower_income_bound,
  COALESCE(IB.ib_upper_bound, 100000) AS upper_income_bound
FROM AggregatedSales AS ASH
LEFT JOIN household_demographics AS HD
  ON HD.hd_demo_sk IN (
    SELECT
      c.c_current_hdemo_sk
    FROM customer AS c
    WHERE
      c.c_current_addr_sk IN (
        SELECT
          ca.ca_address_sk
        FROM customer_address AS ca
        WHERE
          ca.ca_state = ASH.ca_state
      )
  )
LEFT JOIN income_band AS IB
  ON HD.hd_income_band_sk = IB.ib_income_band_sk
WHERE
  ASH.total_sales > 10000
ORDER BY
  ASH.total_sales DESC;
