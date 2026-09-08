WITH SalesData AS (
  SELECT
    ws.ws_item_sk,
    SUM(ws.ws_quantity) AS total_quantity,
    SUM(ws.ws_net_profit) AS total_net_profit,
    ROW_NUMBER() OVER (PARTITION BY ws.ws_item_sk ORDER BY SUM(ws.ws_net_profit) DESC) AS profit_rank
  FROM web_sales AS ws
  JOIN item AS i
    ON ws.ws_item_sk = i.i_item_sk
  WHERE
    i.i_current_price > 0
    AND ws.ws_sold_date_sk IN (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_year = 2023
    )
  GROUP BY
    ws.ws_item_sk
), CustomerData AS (
  SELECT DISTINCT
    c.c_customer_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_purchase_estimate
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  WHERE
    cd.cd_purchase_estimate > 1000
), InventoryData AS (
  SELECT
    inv.inv_item_sk,
    SUM(inv.inv_quantity_on_hand) AS total_inventory
  FROM inventory AS inv
  WHERE
    inv_date_sk = (
      SELECT
        MAX(inv_date_sk)
      FROM inventory
    )
  GROUP BY
    inv.inv_item_sk
)
SELECT
  sd.ws_item_sk,
  COALESCE(sd.total_quantity, 0) AS total_quantity_sold,
  COALESCE(sd.total_net_profit, 0) AS total_net_profit,
  id.total_inventory,
  CASE WHEN sd.profit_rank = 1 THEN 'Top Performer' ELSE 'Regular Performer' END AS performance_status
FROM SalesData AS sd
FULL OUTER JOIN InventoryData AS id
  ON sd.ws_item_sk = id.inv_item_sk
JOIN CustomerData AS cd
  ON cd.c_customer_sk = (
    SELECT
      MAX(ws_bill_customer_sk)
    FROM web_sales
    WHERE
      ws_item_sk = sd.ws_item_sk
  )
WHERE
  NOT id.total_inventory IS NULL
  AND (
    sd.total_net_profit > 1000 OR sd.total_quantity > 100
  )
ORDER BY
  total_net_profit DESC,
  total_quantity_sold DESC;
