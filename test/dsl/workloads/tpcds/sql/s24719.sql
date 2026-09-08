WITH RankedSales AS (
  SELECT
    ws_sold_date_sk,
    ws_item_sk,
    ws_net_profit,
    DENSE_RANK() OVER (PARTITION BY ws_item_sk ORDER BY ws_net_profit DESC) AS profit_rank
  FROM web_sales
  WHERE
    NOT ws_net_profit IS NULL
), CustomerStats AS (
  SELECT
    c.c_customer_id,
    COUNT(DISTINCT ws.ws_order_number) AS total_orders,
    SUM(ws.ws_net_profit) AS total_spent,
    MIN(d.d_date) AS first_order_date,
    MAX(d.d_date) AS last_order_date,
    SUM(
      CASE
        WHEN NOT c.c_birth_year IS NULL
        AND EXTRACT(YEAR FROM CURRENT_DATE) - c.c_birth_year >= 18
        THEN 1
        ELSE 0
      END
    ) AS adult_count
  FROM customer AS c
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  LEFT JOIN date_dim AS d
    ON ws.ws_sold_date_sk = d.d_date_sk
  GROUP BY
    c.c_customer_id
), Promotions AS (
  SELECT
    p.p_promo_id,
    COUNT(cs.cs_order_number) AS promo_count,
    SUM(cs.cs_net_profit) AS promo_profit
  FROM promotion AS p
  JOIN catalog_sales AS cs
    ON p.p_promo_sk = cs.cs_promo_sk
  WHERE
    p.p_discount_active = 'Y'
  GROUP BY
    p.p_promo_id
  HAVING
    COUNT(cs.cs_order_number) > 10
)
SELECT
  cs.c_customer_id,
  cs.total_orders,
  cs.total_spent,
  EXTRACT(year FROM AGE(cs.first_order_date)) AS customer_age,
  pr.promo_count,
  pr.promo_profit,
  MAX(rp.ws_net_profit) AS max_web_profit
FROM CustomerStats AS cs
LEFT JOIN Promotions AS pr
  ON cs.total_spent > 500
LEFT JOIN RankedSales AS rp
  ON cs.c_customer_id = (
    SELECT
      c.c_customer_id
    FROM customer AS c
    WHERE
      c.c_customer_sk = rp.ws_item_sk
  )
WHERE
  COALESCE(cs.adult_count, 0) > 0
GROUP BY
  cs.c_customer_id,
  cs.total_orders,
  cs.total_spent,
  cs.first_order_date,
  cs.last_order_date,
  pr.promo_count,
  pr.promo_profit
HAVING
  NOT MAX(rp.ws_net_profit) IS NULL
ORDER BY
  customer_age DESC,
  cs.total_spent DESC
LIMIT 50;
