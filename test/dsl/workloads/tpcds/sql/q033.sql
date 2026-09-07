/* end query 31 in stream 0 using template query37.tpl */ /* start query 32 in stream 0 using template query98.tpl */
SELECT
  i_item_id,
  i_item_desc,
  i_category,
  i_class,
  i_current_price,
  SUM(ss_ext_sales_price) AS itemrevenue,
  SUM(ss_ext_sales_price) * 100 / SUM(SUM(ss_ext_sales_price)) OVER (PARTITION BY i_class) AS revenueratio
FROM store_sales, item, date_dim
WHERE
  ss_item_sk = i_item_sk
  AND i_category IN ('Electronics', 'Men', 'Books')
  AND ss_sold_date_sk = d_date_sk
  AND d_date BETWEEN CAST('1999-04-10' AS DATE) AND (
    CAST('1999-04-10' AS DATE) + INTERVAL '30 DAY'
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
  revenueratio;
