WITH RankedSales AS (
  SELECT
    ws.ws_order_number,
    ws.ws_item_sk,
    ws.ws_ext_sales_price,
    ROW_NUMBER() OVER (PARTITION BY ws.ws_item_sk ORDER BY ws.ws_sold_date_sk DESC) AS Rank
  FROM web_sales AS ws
  WHERE
    ws.ws_net_profit > (
      SELECT
        AVG(ws_net_profit)
      FROM web_sales
      WHERE
        ws_item_sk = ws.ws_item_sk
    )
), InventoryCheck AS (
  SELECT
    inv.inv_item_sk,
    SUM(inv.inv_quantity_on_hand) AS total_quantity
  FROM inventory AS inv
  GROUP BY
    inv.inv_item_sk
  HAVING
    SUM(inv.inv_quantity_on_hand) < (
      SELECT
        AVG(inv_quantity_on_hand)
      FROM inventory
    )
), CustomerDemographics AS (
  SELECT
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    COUNT(DISTINCT c.c_customer_sk) AS customer_count
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  GROUP BY
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status
), PromotionAnalysis AS (
  SELECT
    p.p_promo_id,
    SUM(ws.ws_net_paid_inc_tax) AS total_revenue,
    COUNT(DISTINCT ws.ws_order_number) AS total_orders,
    SUM(CASE WHEN ws.ws_ship_date_sk IS NULL THEN 1 ELSE 0 END) AS pending_shipments
  FROM promotion AS p
  LEFT JOIN web_sales AS ws
    ON p.p_promo_sk = ws.ws_promo_sk
  WHERE
    p.p_discount_active = 'Y'
  GROUP BY
    p.p_promo_id
)
SELECT
  cd.cd_gender,
  cd.cd_marital_status,
  SUM(RS.ws_ext_sales_price) AS total_sales,
  COUNT(DISTINCT RS.ws_order_number) AS order_count,
  I.total_quantity AS inventory_quantity,
  PA.total_revenue AS promotion_revenue,
  PA.pending_shipments
FROM RankedSales AS RS
JOIN CustomerDemographics AS cd
  ON RS.ws_item_sk IN (
    SELECT
      ic.inv_item_sk
    FROM InventoryCheck AS ic
  )
LEFT JOIN InventoryCheck AS I
  ON I.inv_item_sk = RS.ws_item_sk
LEFT JOIN PromotionAnalysis AS PA
  ON PA.p_promo_id IN (
    SELECT DISTINCT
      p.p_promo_id
    FROM promotion AS p
    WHERE
      p.p_start_date_sk < (
        SELECT
          MAX(d.d_date_sk)
        FROM date_dim AS d
      )
      AND p.p_end_date_sk > (
        SELECT
          MIN(d.d_date_sk)
        FROM date_dim AS d
      )
  )
WHERE
  RS.Rank = 1
GROUP BY
  cd.cd_gender,
  cd.cd_marital_status,
  I.total_quantity,
  PA.total_revenue,
  PA.pending_shipments
HAVING
  (
    NOT SUM(RS.ws_ext_sales_price) IS NULL OR COUNT(DISTINCT RS.ws_order_number) > 0
  )
ORDER BY
  cd.cd_gender,
  cd.cd_marital_status;
