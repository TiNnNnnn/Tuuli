/* end query 51 in stream 0 using template query11.tpl */ /* start query 52 in stream 0 using template query93.tpl */
SELECT
  ss_customer_sk,
  SUM(act_sales) AS sumsales
FROM (
  SELECT
    ss_item_sk,
    ss_ticket_number,
    ss_customer_sk,
    CASE
      WHEN NOT sr_return_quantity IS NULL
      THEN (
        ss_quantity - sr_return_quantity
      ) * ss_sales_price
      ELSE (
        ss_quantity * ss_sales_price
      )
    END AS act_sales
  FROM store_sales
  LEFT OUTER JOIN store_returns
    ON (
      sr_item_sk = ss_item_sk AND sr_ticket_number = ss_ticket_number
    ), reason
  WHERE
    sr_reason_sk = r_reason_sk AND r_reason_desc = 'reason 72'
) AS t
GROUP BY
  ss_customer_sk
ORDER BY
  sumsales,
  ss_customer_sk
LIMIT 100;
