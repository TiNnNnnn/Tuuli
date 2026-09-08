WITH AddressAggregation AS (
  SELECT
    ca_city,
    ca_state,
    COUNT(*) AS address_count,
    STRING_AGG(ca_street_name || ' ' || ca_street_number || ' ' || ca_street_type, '; ') AS full_address
  FROM customer_address
  GROUP BY
    ca_city,
    ca_state
), CustomerAggregation AS (
  SELECT
    cd_gender,
    COUNT(c_customer_sk) AS customer_count,
    STRING_AGG(c_first_name || ' ' || c_last_name, ', ') AS full_customer_names
  FROM customer
  JOIN customer_demographics
    ON c_current_cdemo_sk = cd_demo_sk
  GROUP BY
    cd_gender
)
SELECT
  a.ca_city,
  a.ca_state,
  a.address_count,
  a.full_address,
  c.cd_gender,
  c.customer_count,
  c.full_customer_names
FROM AddressAggregation AS a
FULL OUTER JOIN CustomerAggregation AS c
  ON a.ca_state = c.cd_gender
ORDER BY
  a.ca_city,
  c.cd_gender;
