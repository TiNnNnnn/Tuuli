WITH processed_addresses AS (
  SELECT
    CONCAT(
      ca_street_number,
      ' ',
      ca_street_name,
      ' ',
      ca_street_type,
      COALESCE(CONCAT(' Suite ', ca_suite_number), '')
    ) AS full_address,
    ca_city,
    ca_state,
    ca_zip,
    ca_country
  FROM customer_address
), address_count AS (
  SELECT
    full_address,
    ca_city,
    ca_state,
    COUNT(*) AS address_frequency
  FROM processed_addresses
  GROUP BY
    full_address,
    ca_city,
    ca_state
), demographics AS (
  SELECT
    cd_gender,
    cd_marital_status,
    cd_education_status,
    cd_purchase_estimate,
    c_customer_sk,
    CONCAT(c_first_name, ' ', c_last_name) AS full_name
  FROM customer
  JOIN customer_demographics
    ON c_current_cdemo_sk = cd_demo_sk
)
SELECT
  da.full_address,
  da.ca_city,
  da.ca_state,
  da.ca_zip,
  da.ca_country,
  ac.address_frequency,
  dm.full_name,
  dm.cd_gender,
  dm.cd_marital_status,
  dm.cd_education_status,
  dm.cd_purchase_estimate
FROM address_count AS ac
JOIN processed_addresses AS da
  ON ac.full_address = da.full_address
JOIN demographics AS dm
  ON RANDOM() < 0.1
ORDER BY
  ac.address_frequency DESC,
  dm.cd_purchase_estimate DESC
FETCH FIRST 100 ROWS ONLY;
