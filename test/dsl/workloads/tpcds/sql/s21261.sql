WITH CustomerSummary AS (
  SELECT
    c.c_customer_sk,
    c.c_customer_id,
    cd.cd_gender,
    cd.cd_marital_status,
    COALESCE(SUM(CASE WHEN NOT ws.ws_sold_date_sk IS NULL THEN ws.ws_sales_price ELSE 0 END), 0) AS total_sales,
    COUNT(DISTINCT ws.ws_order_number) AS order_count
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_bill_customer_sk
  GROUP BY
    c.c_customer_sk,
    c.c_customer_id,
    cd.cd_gender,
    cd.cd_marital_status
), InventorySummary AS (
  SELECT
    inv.inv_item_sk,
    SUM(inv.inv_quantity_on_hand) AS total_quantity
  FROM inventory AS inv
  GROUP BY
    inv.inv_item_sk
), BizarreSales AS (
  SELECT
    ws.ws_item_sk,
    SUM(ws.ws_sales_price) AS bizarre_total
  FROM web_sales AS ws
  WHERE
    ws.ws_quantity > 0
  GROUP BY
    ws.ws_item_sk
  HAVING
    SUM(ws.ws_sales_price) > (
      SELECT
        AVG(ws2.ws_sales_price)
      FROM web_sales AS ws2
    )
), RankedSales AS (
  SELECT
    c.c_customer_sk,
    cs.total_sales,
    RANK() OVER (PARTITION BY c.c_customer_sk ORDER BY cs.total_sales DESC) AS sales_rank
  FROM CustomerSummary AS cs
  JOIN customer AS c
    ON cs.c_customer_sk = c.c_customer_sk
)
SELECT
  cs.c_customer_id,
  cs.total_sales,
  cs.order_count,
  COALESCE(b.bizarre_total, 0) AS bizarre_sales_total,
  RANK() OVER (ORDER BY cs.total_sales DESC) AS overall_rank,
  CASE
    WHEN cs.total_sales > (
      SELECT
        AVG(total_sales)
      FROM CustomerSummary
    )
    THEN 'Above Average'
    ELSE 'Below Average'
  END AS sales_performance,
  NULLIF(
    (
      SELECT
        MAX(total_quantity)
      FROM InventorySummary
      WHERE
        inv_item_sk IN (
          SELECT DISTINCT
            ws.ws_item_sk
          FROM web_sales AS ws
          WHERE
            ws.ws_bill_customer_sk = cs.c_customer_sk
        )
    ),
    0
  ) AS max_inventory_for_customer
FROM CustomerSummary AS cs
LEFT JOIN BizarreSales AS b
  ON cs.c_customer_sk = (
    SELECT
      ws_bill_customer_sk
    FROM web_sales
    WHERE
      ws_item_sk = b.ws_item_sk
    LIMIT 1
  )
WHERE
  cs.total_sales <> ALL (
    SELECT
      total_sales
    FROM CustomerSummary
    WHERE
      total_sales < cs.total_sales
  )
ORDER BY
  cs.total_sales DESC;
