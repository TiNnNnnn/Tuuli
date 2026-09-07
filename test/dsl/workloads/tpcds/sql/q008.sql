/* end query 6 in stream 0 using template query80.tpl */ /* start query 7 in stream 0 using template query32.tpl */
SELECT
  SUM(cs_ext_discount_amt) AS "excess discount amount"
FROM catalog_sales, item, date_dim
WHERE
  i_manufact_id = 382
  AND i_item_sk = cs_item_sk
  AND d_date BETWEEN '1999-02-09' AND (
    CAST('1999-02-09' AS DATE) + INTERVAL '90 DAY'
  )
  AND d_date_sk = cs_sold_date_sk
  AND cs_ext_discount_amt > (
    SELECT
      1.3 * AVG(cs_ext_discount_amt)
    FROM catalog_sales, date_dim
    WHERE
      cs_item_sk = i_item_sk
      AND d_date BETWEEN '1999-02-09' AND (
        CAST('1999-02-09' AS DATE) + INTERVAL '90 DAY'
      )
      AND d_date_sk = cs_sold_date_sk
  )
LIMIT 100;
