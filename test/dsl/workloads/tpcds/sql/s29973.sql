WITH RankedCustomers AS (
  SELECT
    c.c_customer_sk,
    CONCAT(c.c_first_name, ' ', c.c_last_name) AS full_name,
    cd.cd_gender,
    cd.cd_marital_status,
    STRING_AGG(DISTINCT ca.ca_city, ', ') AS cities,
    COUNT(DISTINCT ss.ss_item_sk) AS total_items_purchased,
    SUM(ss.ss_sales_price) AS total_spent
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  LEFT JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  LEFT JOIN customer_address AS ca
    ON c.c_current_addr_sk = ca.ca_address_sk
  GROUP BY
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    cd.cd_gender,
    cd.cd_marital_status
), CustomerStats AS (
  SELECT
    *,
    RANK() OVER (ORDER BY total_spent DESC) AS rank_by_spending,
    RANK() OVER (ORDER BY total_items_purchased DESC) AS rank_by_items
  FROM RankedCustomers
)
SELECT
  full_name,
  cd_gender,
  cd_marital_status,
  cities,
  total_items_purchased,
  total_spent,
  rank_by_spending,
  rank_by_items
FROM CustomerStats
WHERE
  rank_by_spending <= 10 OR rank_by_items <= 10
ORDER BY
  rank_by_spending,
  rank_by_items;
