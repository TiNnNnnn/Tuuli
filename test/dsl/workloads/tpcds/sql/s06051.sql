WITH sales_summary AS (
  SELECT
    c.c_customer_id,
    SUM(ss.ss_quantity) AS total_quantity,
    SUM(ss.ss_net_paid) AS total_net_paid,
    AVG(ss.ss_sales_price) AS avg_sales_price,
    COUNT(DISTINCT ss.ss_ticket_number) AS total_transactions,
    d.d_year
  FROM store_sales AS ss
  JOIN customer AS c
    ON ss.ss_customer_sk = c.c_customer_sk
  JOIN date_dim AS d
    ON ss.ss_sold_date_sk = d.d_date_sk
  WHERE
    d.d_year BETWEEN 2018 AND 2023
  GROUP BY
    c.c_customer_id,
    d.d_year
), customer_demographics_summary AS (
  SELECT
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    COUNT(DISTINCT ss.c_customer_id) AS customer_count,
    SUM(ss.total_quantity) AS quantity_sold,
    SUM(ss.total_net_paid) AS total_revenue
  FROM customer_demographics AS cd
  JOIN sales_summary AS ss
    ON cd.cd_demo_sk = (
      SELECT
        c.c_current_cdemo_sk
      FROM customer AS c
      WHERE
        c.c_customer_id = ss.c_customer_id
    )
  GROUP BY
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status
), ranked_demographics AS (
  SELECT
    cd.cd_gender,
    cd.cd_marital_status,
    customer_count,
    quantity_sold,
    total_revenue,
    RANK() OVER (PARTITION BY cd.cd_gender ORDER BY total_revenue DESC) AS revenue_rank
  FROM customer_demographics_summary AS cd
)
SELECT
  cd_gender,
  cd_marital_status,
  customer_count,
  quantity_sold,
  total_revenue
FROM ranked_demographics
WHERE
  revenue_rank <= 10
ORDER BY
  cd_gender,
  total_revenue DESC;
