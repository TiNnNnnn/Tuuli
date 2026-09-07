/* end query 64 in stream 0 using template query12.tpl */ /* start query 65 in stream 0 using template query20.tpl */
SELECT
  i_item_id,
  i_item_desc,
  i_category,
  i_class,
  i_current_price,
  SUM(cs_ext_sales_price) AS itemrevenue,
  SUM(cs_ext_sales_price) * 100 / SUM(SUM(cs_ext_sales_price)) OVER (PARTITION BY i_class) AS revenueratio
FROM catalog_sales, item, date_dim
WHERE
  cs_item_sk = i_item_sk
  AND i_category IN ('Books', 'Children', 'Sports')
  AND cs_sold_date_sk = d_date_sk
  AND d_date BETWEEN CAST('2001-04-30' AS DATE) AND (
    CAST('2001-04-30' AS DATE) + INTERVAL '30 DAY'
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
