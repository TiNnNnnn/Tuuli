WITH sales_summary AS (
  SELECT
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    COALESCE(SUM(ss.ss_net_profit), 0) AS total_store_profit,
    COALESCE(SUM(ws.ws_net_profit), 0) AS total_web_profit,
    (
      SELECT
        COUNT(*)
      FROM store_returns AS sr
      WHERE
        sr.sr_customer_sk = c.c_customer_sk
    ) AS total_store_returns,
    (
      SELECT
        COUNT(*)
      FROM web_returns AS wr
      WHERE
        wr.wr_returning_customer_sk = c.c_customer_sk
    ) AS total_web_returns
  FROM customer AS c
  LEFT JOIN store_sales AS ss
    ON c.c_customer_sk = ss.ss_customer_sk
  LEFT JOIN web_sales AS ws
    ON c.c_customer_sk = ws.ws_ship_customer_sk
  GROUP BY
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name
), ranked_sales AS (
  SELECT
    *,
    RANK() OVER (ORDER BY total_store_profit + total_web_profit DESC) AS profit_rank
  FROM sales_summary
)
SELECT
  r.c_customer_sk,
  r.c_first_name,
  r.c_last_name,
  r.total_store_profit,
  r.total_web_profit,
  r.total_store_returns,
  r.total_web_returns,
  CASE
    WHEN r.total_store_profit > r.total_web_profit
    THEN 'Store'
    WHEN r.total_web_profit > r.total_store_profit
    THEN 'Web'
    ELSE 'Equal'
  END AS preferred_channel
FROM ranked_sales AS r
WHERE
  r.profit_rank <= 10
ORDER BY
  r.profit_rank;
