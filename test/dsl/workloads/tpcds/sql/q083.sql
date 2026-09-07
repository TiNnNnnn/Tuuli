/* end query 79 in stream 0 using template query73.tpl */ /* start query 80 in stream 0 using template query84.tpl */
SELECT
  c_customer_id AS customer_id,
  COALESCE(c_last_name, '') || ', ' || COALESCE(c_first_name, '') AS customername
FROM customer, customer_address, customer_demographics, household_demographics, income_band, store_returns
WHERE
  ca_city = 'Salem'
  AND c_current_addr_sk = ca_address_sk
  AND ib_lower_bound >= 59631
  AND ib_upper_bound <= 59631 + 50000
  AND ib_income_band_sk = hd_income_band_sk
  AND cd_demo_sk = c_current_cdemo_sk
  AND hd_demo_sk = c_current_hdemo_sk
  AND sr_cdemo_sk = cd_demo_sk
ORDER BY
  c_customer_id
LIMIT 100;
