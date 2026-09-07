/* end query 63 in stream 0 using template query8.tpl */ /* start query 64 in stream 0 using template query12.tpl */
SELECT
  i_item_id,
  i_item_desc,
  i_category,
  i_class,
  i_current_price,
  SUM(ws_ext_sales_price) AS itemrevenue,
  SUM(ws_ext_sales_price) * 100 / SUM(SUM(ws_ext_sales_price)) OVER (PARTITION BY i_class) AS revenueratio
FROM web_sales, item, date_dim
WHERE
  ws_item_sk = i_item_sk
  AND i_category IN ('Shoes', 'Home', 'Women')
  AND ws_sold_date_sk = d_date_sk
  AND d_date BETWEEN CAST('2000-05-16' AS DATE) AND (
    CAST('2000-05-16' AS DATE) + INTERVAL '30 DAY'
  )
GROUP BY
  i_item_id,
  i_item_desc,
  i_category,
  i_class,
  i_current_price
ORDER BY
  i_category,
  i_class,
  i_item_id,
  i_item_desc,
  revenueratio
LIMIT 100;
