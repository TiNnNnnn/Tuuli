WITH RankedAddresses AS (
  SELECT
    ca.ca_address_sk,
    ca.ca_address_id,
    ca.ca_city,
    ca.ca_state,
    ROW_NUMBER() OVER (PARTITION BY ca.ca_city ORDER BY ca.ca_address_sk) AS city_rank
  FROM customer_address AS ca
), CustomerDetails AS (
  SELECT
    c.c_customer_sk,
    CONCAT(c.c_first_name, ' ', c.c_last_name) AS full_name,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_education_status
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
), FinalBenchmark AS (
  SELECT
    ra.ca_address_id,
    ra.ca_city,
    ra.ca_state,
    cd.full_name,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_education_status
  FROM RankedAddresses AS ra
  JOIN CustomerDetails AS cd
    ON ra.ca_address_sk = cd.c_customer_sk
  WHERE
    ra.city_rank <= 10
)
SELECT
  fb.ca_city,
  COUNT(*) AS customer_count,
  MAX(fb.cd_education_status) AS highest_education,
  STRING_AGG(fb.full_name, ', ') AS customer_names
FROM FinalBenchmark AS fb
GROUP BY
  fb.ca_city
ORDER BY
  customer_count DESC;
