SELECT
  CA.ca_address_id,
  CA.ca_street_name,
  CA.ca_city,
  CA.ca_state,
  CD.cd_gender,
  CD.cd_marital_status,
  COUNT(DISTINCT C.c_customer_sk) AS customer_count,
  SUM(CD.cd_purchase_estimate) AS total_purchase_estimate,
  STRING_AGG(DISTINCT CONCAT_WS(', ', C.c_first_name, C.c_last_name), '; ') AS customer_names
FROM customer_address AS CA
JOIN customer AS C
  ON CA.ca_address_sk = C.c_current_addr_sk
JOIN customer_demographics AS CD
  ON C.c_current_cdemo_sk = CD.cd_demo_sk
WHERE
  CA.ca_state IN ('CA', 'NY') AND C.c_birth_year BETWEEN 1980 AND 1990
GROUP BY
  CA.ca_address_id,
  CA.ca_street_name,
  CA.ca_city,
  CA.ca_state,
  CD.cd_gender,
  CD.cd_marital_status
HAVING
  COUNT(DISTINCT C.c_customer_sk) > 5
ORDER BY
  total_purchase_estimate DESC
LIMIT 10;
