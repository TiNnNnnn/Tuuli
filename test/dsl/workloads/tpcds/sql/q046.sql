/* end query 44 in stream 0 using template query92.tpl */ /* start query 45 in stream 0 using template query3.tpl */
SELECT
  dt.d_year,
  item.i_brand_id AS brand_id,
  item.i_brand AS brand,
  SUM(ss_sales_price) AS sum_agg
FROM date_dim AS dt, store_sales, item
WHERE
  dt.d_date_sk = store_sales.ss_sold_date_sk
  AND store_sales.ss_item_sk = item.i_item_sk
  AND item.i_manufact_id = 882
  AND dt.d_moy = 11
GROUP BY
  dt.d_year,
  item.i_brand,
  item.i_brand_id
ORDER BY
  dt.d_year,
  sum_agg DESC,
  brand_id
LIMIT 100;
