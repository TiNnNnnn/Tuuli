WITH CustomerData AS (
  SELECT
    c.c_first_name AS first_name,
    c.c_last_name AS last_name,
    ca.ca_city AS city,
    ca.ca_state AS state,
    cd.cd_gender AS gender,
    STRING_AGG(DISTINCT CONCAT(p.p_promo_name, ' (', p.p_promo_id, ')'), ',') AS promotions,
    COUNT(DISTINCT cs.cs_order_number) AS total_orders,
    SUM(cs.cs_net_profit) AS total_profit
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  JOIN customer_address AS ca
    ON c.c_current_addr_sk = ca.ca_address_sk
  LEFT JOIN catalog_sales AS cs
    ON c.c_customer_sk = cs.cs_bill_customer_sk
  LEFT JOIN promotion AS p
    ON cs.cs_promo_sk = p.p_promo_sk
  GROUP BY
    c.c_first_name,
    c.c_last_name,
    ca.ca_city,
    ca.ca_state,
    cd.cd_gender
), RankedCustomers AS (
  SELECT
    first_name,
    last_name,
    city,
    state,
    gender,
    promotions,
    total_orders,
    total_profit,
    RANK() OVER (PARTITION BY gender ORDER BY total_profit DESC) AS profit_rank
  FROM CustomerData
)
SELECT
  first_name,
  last_name,
  city,
  state,
  gender,
  promotions,
  total_orders,
  total_profit,
  profit_rank
FROM RankedCustomers
WHERE
  profit_rank <= 10
ORDER BY
  gender,
  total_profit DESC;
