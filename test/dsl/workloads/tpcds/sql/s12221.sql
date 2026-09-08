SELECT
  w.w_warehouse_name,
  COUNT(ss.ss_ticket_number) AS total_sales,
  SUM(ss.ss_net_profit) AS total_profit
FROM warehouse AS w
JOIN store AS s
  ON w.w_warehouse_sk = s.s_store_sk
JOIN store_sales AS ss
  ON s.s_store_sk = ss.ss_store_sk
WHERE
  ss.ss_sold_date_sk BETWEEN 2451565 AND 2451592
GROUP BY
  w.w_warehouse_name
ORDER BY
  total_profit DESC
LIMIT 10;
