WITH RECURSIVE SalesHierarchy AS (
  SELECT
    c.c_customer_sk,
    c.c_first_name,
    c.c_last_name,
    c.c_current_cdemo_sk,
    1 AS Level
  FROM customer AS c
  WHERE
    NOT c.c_customer_sk IS NULL
  UNION ALL
  SELECT
    sh.c_customer_sk,
    sh.c_first_name,
    sh.c_last_name,
    sh.c_current_cdemo_sk,
    Level + 1
  FROM customer AS sh
  INNER JOIN SalesHierarchy AS sh_parent
    ON sh.c_current_cdemo_sk = sh_parent.c_current_cdemo_sk
  WHERE
    sh.c_customer_sk <> sh_parent.c_customer_sk
), AggregatedSales AS (
  SELECT
    ss.ss_customer_sk,
    COUNT(ss.ss_ticket_number) AS Total_Sales,
    SUM(ss.ss_net_paid_inc_tax) AS Total_Sales_Amount
  FROM store_sales AS ss
  WHERE
    ss.ss_sold_date_sk BETWEEN 2450000 AND 2450600
  GROUP BY
    ss.ss_customer_sk
), FilteredDemographics AS (
  SELECT
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    COALESCE(SUM(total_sales.Total_Sales), 0) AS Total_Sales_Count,
    COALESCE(SUM(total_sales.Total_Sales_Amount), 0) AS Total_Sales_Sum
  FROM customer_demographics AS cd
  LEFT JOIN AggregatedSales AS total_sales
    ON cd.cd_demo_sk = (
      SELECT
        c.c_current_cdemo_sk
      FROM customer AS c
      WHERE
        c.c_customer_sk = total_sales.ss_customer_sk
    )
  GROUP BY
    cd.cd_demo_sk,
    cd.cd_gender,
    cd.cd_marital_status
  HAVING
    SUM(total_sales.Total_Sales) > 10
), RankedDemo AS (
  SELECT
    fd.cd_gender,
    fd.cd_marital_status,
    fd.Total_Sales_Count,
    fd.Total_Sales_Sum,
    RANK() OVER (PARTITION BY fd.cd_gender ORDER BY fd.Total_Sales_Sum DESC) AS Sales_Rank
  FROM FilteredDemographics AS fd
)
SELECT
  rd.cd_gender,
  rd.cd_marital_status,
  rd.Total_Sales_Count,
  rd.Total_Sales_Sum,
  CASE WHEN rd.Sales_Rank <= 5 THEN 'Top 5' ELSE 'Others' END AS Sales_Classification
FROM RankedDemo AS rd
WHERE
  NOT rd.Total_Sales_Sum IS NULL
ORDER BY
  rd.cd_gender,
  rd.Sales_Rank;
