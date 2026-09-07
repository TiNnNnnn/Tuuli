/* end query 30 in stream 0 using template query59.tpl */ /* start query 31 in stream 0 using template query37.tpl */
SELECT
  i_item_id,
  i_item_desc,
  i_current_price
FROM item, inventory, date_dim, catalog_sales
WHERE
  i_current_price BETWEEN 64 AND 64 + 30
  AND inv_item_sk = i_item_sk
  AND d_date_sk = inv_date_sk
  AND d_date BETWEEN CAST('2001-04-08' AS DATE) AND (
    CAST('2001-04-08' AS DATE) + INTERVAL '60 DAY'
  )
  AND i_manufact_id IN (969, 961, 688, 779)
  AND inv_quantity_on_hand BETWEEN 100 AND 500
  AND cs_item_sk = i_item_sk
GROUP BY
  i_item_id,
  i_item_desc,
  i_current_price
ORDER BY
  i_item_id
LIMIT 100;
