/* end query 81 in stream 0 using template query54.tpl */ /* start query 82 in stream 0 using template query55.tpl */
SELECT
  i_brand_id AS brand_id,
  i_brand AS brand,
  SUM(ss_ext_sales_price) AS ext_price
FROM date_dim, store_sales, item
WHERE
  d_date_sk = ss_sold_date_sk
  AND ss_item_sk = i_item_sk
  AND i_manager_id = 69
  AND d_moy = 12
  AND d_year = 1998
GROUP BY
  i_brand,
  i_brand_id
ORDER BY
  ext_price DESC,
  i_brand_id
LIMIT 100;
