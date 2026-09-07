/* end query 66 in stream 0 using template query88.tpl */ /* start query 67 in stream 0 using template query82.tpl */
SELECT
  i_item_id,
  i_item_desc,
  i_current_price
FROM item, inventory, date_dim, store_sales
WHERE
  i_current_price BETWEEN 86 AND 86 + 30
  AND inv_item_sk = i_item_sk
  AND d_date_sk = inv_date_sk
  AND d_date BETWEEN CAST('2000-01-29' AS DATE) AND (
    CAST('2000-01-29' AS DATE) + INTERVAL '60 DAY'
  )
  AND i_manufact_id IN (299, 89, 721, 855)
  AND inv_quantity_on_hand BETWEEN 100 AND 500
  AND ss_item_sk = i_item_sk
GROUP BY
  i_item_id,
  i_item_desc,
  i_current_price
ORDER BY
  i_item_id
LIMIT 100;
