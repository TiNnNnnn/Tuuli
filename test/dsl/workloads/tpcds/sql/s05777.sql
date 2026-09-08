WITH TotalSales AS (
  SELECT
    d.d_year,
    SUM(ws_ext_sales_price) AS total_sales,
    SUM(ws_ext_discount_amt) AS total_discount,
    SUM(ws_ext_tax) AS total_tax,
    COUNT(DISTINCT ws_order_number) AS total_orders
  FROM web_sales
  JOIN date_dim AS d
    ON ws_sold_date_sk = d.d_date_sk
  GROUP BY
    d.d_year
), CustomerDemoStats AS (
  SELECT
    cd_marital_status,
    COUNT(DISTINCT c.c_customer_sk) AS customer_count,
    AVG(cd_purchase_estimate) AS avg_purchase_estimate
  FROM customer AS c
  JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
  GROUP BY
    cd_marital_status
), WarehouseSales AS (
  SELECT
    w.w_warehouse_name,
    SUM(ws_ext_sales_price) AS warehouse_sales
  FROM web_sales AS ws
  JOIN warehouse AS w
    ON ws.ws_warehouse_sk = w.w_warehouse_sk
  GROUP BY
    w.w_warehouse_name
)
SELECT
  ts.d_year,
  ts.total_sales,
  ts.total_discount,
  ts.total_tax,
  ts.total_orders,
  cds.cd_marital_status,
  cds.customer_count,
  cds.avg_purchase_estimate,
  wws.warehouse_sales
FROM TotalSales AS ts
JOIN CustomerDemoStats AS cds
  ON ts.d_year = (
    SELECT
      MAX(d_year)
    FROM TotalSales
  )
JOIN WarehouseSales AS wws
  ON wws.warehouse_sales = (
    SELECT
      MAX(warehouse_sales)
    FROM WarehouseSales
  )
ORDER BY
  ts.d_year DESC;
