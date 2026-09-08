WITH RankedSales AS (
  SELECT
    ws.ws_item_sk,
    SUM(ws.ws_quantity) AS total_quantity,
    SUM(ws.ws_net_profit) AS total_net_profit,
    RANK() OVER (PARTITION BY ws.ws_ship_mode_sk ORDER BY SUM(ws.ws_net_profit) DESC) AS profit_rank
  FROM web_sales AS ws
  JOIN item AS i
    ON ws.ws_item_sk = i.i_item_sk
  JOIN promotion AS p
    ON ws.ws_promo_sk = p.p_promo_sk
  WHERE
    p.p_start_date_sk <= (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_date = CAST('2002-10-01' AS DATE)
    )
    AND p.p_end_date_sk >= (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_date = CAST('2002-10-01' AS DATE)
    )
    AND ws.ws_sold_date_sk >= (
      SELECT
        MAX(d_date_sk)
      FROM date_dim
      WHERE
        d_date = CAST('2002-10-01' AS DATE) - INTERVAL '30 DAYS'
    )
  GROUP BY
    ws.ws_item_sk,
    ws.ws_ship_mode_sk
), TopRanked AS (
  SELECT
    rs.ws_item_sk,
    rs.total_quantity,
    rs.total_net_profit,
    rs.profit_rank,
    i.i_product_name,
    i.i_brand,
    i.i_category
  FROM RankedSales AS rs
  JOIN item AS i
    ON rs.ws_item_sk = i.i_item_sk
  WHERE
    rs.profit_rank <= 10
)
SELECT
  tra.i_product_name,
  tra.i_brand,
  tra.i_category,
  tra.total_quantity,
  tra.total_net_profit
FROM TopRanked AS tra
ORDER BY
  tra.total_net_profit DESC;
