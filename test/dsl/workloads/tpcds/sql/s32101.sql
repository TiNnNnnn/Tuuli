WITH RECURSIVE AggregatedSales AS (
  SELECT
    ws_item_sk,
    SUM(ws_quantity) AS total_quantity,
    SUM(ws_net_paid) AS total_net_paid,
    ROW_NUMBER() OVER (PARTITION BY ws_item_sk ORDER BY SUM(ws_net_paid) DESC) AS rank
  FROM web_sales
  WHERE
    ws_sold_date_sk BETWEEN (
      SELECT
        MIN(d_date_sk)
      FROM date_dim
    ) AND (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
    )
  GROUP BY
    ws_item_sk
), TopSales AS (
  SELECT
    item.i_item_id,
    COALESCE(ad.ca_city, 'Unknown') AS city,
    SUM(a.total_quantity) AS total_quantity_sold,
    SUM(a.total_net_paid) AS total_revenue
  FROM AggregatedSales AS a
  JOIN item AS item
    ON a.ws_item_sk = item.i_item_sk
  LEFT JOIN customer AS cust
    ON cust.c_customer_sk = (
      SELECT
        ws_bill_customer_sk
      FROM web_sales
      WHERE
        ws_item_sk = a.ws_item_sk
      ORDER BY
        ws_net_paid DESC
      LIMIT 1
    )
  LEFT JOIN customer_address AS ad
    ON cust.c_current_addr_sk = ad.ca_address_sk
  GROUP BY
    item.i_item_id,
    ad.ca_city
)
SELECT
  t.city,
  COUNT(*) AS item_count,
  AVG(t.total_revenue) AS average_revenue,
  MAX(t.total_quantity_sold) AS max_quantity_sold
FROM TopSales AS t
GROUP BY
  t.city
HAVING
  COUNT(*) > 5
ORDER BY
  average_revenue DESC
LIMIT 10
OFFSET 5;
