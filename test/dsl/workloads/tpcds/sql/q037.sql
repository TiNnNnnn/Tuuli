/* end query 35 in stream 0 using template query67.tpl */ /* start query 36 in stream 0 using template query28.tpl */
SELECT
  *
FROM (
  SELECT
    AVG(ss_list_price) AS B1_LP,
    COUNT(ss_list_price) AS B1_CNT,
    COUNT(DISTINCT ss_list_price) AS B1_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 0 AND 5
    AND (
      ss_list_price BETWEEN 16 AND 16 + 10
      OR ss_coupon_amt BETWEEN 834 AND 834 + 1000
      OR ss_wholesale_cost BETWEEN 2 AND 2 + 20
    )
) AS B1, (
  SELECT
    AVG(ss_list_price) AS B2_LP,
    COUNT(ss_list_price) AS B2_CNT,
    COUNT(DISTINCT ss_list_price) AS B2_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 6 AND 10
    AND (
      ss_list_price BETWEEN 115 AND 115 + 10
      OR ss_coupon_amt BETWEEN 3251 AND 3251 + 1000
      OR ss_wholesale_cost BETWEEN 59 AND 59 + 20
    )
) AS B2, (
  SELECT
    AVG(ss_list_price) AS B3_LP,
    COUNT(ss_list_price) AS B3_CNT,
    COUNT(DISTINCT ss_list_price) AS B3_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 11 AND 15
    AND (
      ss_list_price BETWEEN 141 AND 141 + 10
      OR ss_coupon_amt BETWEEN 1420 AND 1420 + 1000
      OR ss_wholesale_cost BETWEEN 62 AND 62 + 20
    )
) AS B3, (
  SELECT
    AVG(ss_list_price) AS B4_LP,
    COUNT(ss_list_price) AS B4_CNT,
    COUNT(DISTINCT ss_list_price) AS B4_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 16 AND 20
    AND (
      ss_list_price BETWEEN 190 AND 190 + 10
      OR ss_coupon_amt BETWEEN 4616 AND 4616 + 1000
      OR ss_wholesale_cost BETWEEN 64 AND 64 + 20
    )
) AS B4, (
  SELECT
    AVG(ss_list_price) AS B5_LP,
    COUNT(ss_list_price) AS B5_CNT,
    COUNT(DISTINCT ss_list_price) AS B5_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 21 AND 25
    AND (
      ss_list_price BETWEEN 57 AND 57 + 10
      OR ss_coupon_amt BETWEEN 962 AND 962 + 1000
      OR ss_wholesale_cost BETWEEN 70 AND 70 + 20
    )
) AS B5, (
  SELECT
    AVG(ss_list_price) AS B6_LP,
    COUNT(ss_list_price) AS B6_CNT,
    COUNT(DISTINCT ss_list_price) AS B6_CNTD
  FROM store_sales
  WHERE
    ss_quantity BETWEEN 26 AND 30
    AND (
      ss_list_price BETWEEN 181 AND 181 + 10
      OR ss_coupon_amt BETWEEN 2264 AND 2264 + 1000
      OR ss_wholesale_cost BETWEEN 3 AND 3 + 20
    )
) AS B6
LIMIT 100;
