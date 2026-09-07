/* end query 54 in stream 0 using template query38.tpl */ /* start query 55 in stream 0 using template query22.tpl */
SELECT
  i_product_name,
  i_brand,
  i_class,
  i_category,
  AVG(inv_quantity_on_hand) AS qoh
FROM inventory, date_dim, item
WHERE
  inv_date_sk = d_date_sk
  AND inv_item_sk = i_item_sk
  AND d_month_seq BETWEEN 1179 AND 1179 + 11
GROUP BY
ROLLUP (
  i_product_name,
  i_brand,
  i_class,
  i_category
)
ORDER BY
  qoh,
  i_product_name,
  i_brand,
  i_class,
  i_category
LIMIT 100;
