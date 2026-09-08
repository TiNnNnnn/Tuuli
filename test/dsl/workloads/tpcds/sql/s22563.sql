WITH CustomerReturns AS (
  SELECT
    sr_customer_sk,
    COUNT(DISTINCT sr_ticket_number) AS return_count,
    SUM(sr_return_amt) AS total_return_amt
  FROM store_returns
  WHERE
    sr_return_quantity > 0
  GROUP BY
    sr_customer_sk
), WebReturns AS (
  SELECT
    wr_returning_customer_sk,
    COUNT(DISTINCT wr_order_number) AS web_return_count,
    SUM(wr_return_amt) AS total_web_return_amt
  FROM web_returns
  WHERE
    wr_return_quantity > 0
  GROUP BY
    wr_returning_customer_sk
), CombinedReturns AS (
  SELECT
    COALESCE(cr.sr_customer_sk, wr.wr_returning_customer_sk) AS customer_sk,
    COALESCE(cr.return_count, 0) AS store_return_count,
    COALESCE(wr.web_return_count, 0) AS web_return_count,
    COALESCE(cr.total_return_amt, 0) + COALESCE(wr.total_web_return_amt, 0) AS total_combined_return_amt
  FROM CustomerReturns AS cr
  FULL OUTER JOIN WebReturns AS wr
    ON cr.sr_customer_sk = wr.wr_returning_customer_sk
), CustomerDemographics AS (
  SELECT
    c.c_customer_sk,
    cd.cd_gender,
    cd.cd_marital_status,
    cd.cd_credit_rating,
    cd.cd_purchase_estimate,
    CASE
      WHEN cd.cd_purchase_estimate IS NULL
      THEN 'Unknown'
      ELSE CASE
        WHEN cd.cd_purchase_estimate < 1000
        THEN 'Low'
        WHEN cd.cd_purchase_estimate BETWEEN 1000 AND 5000
        THEN 'Medium'
        ELSE 'High'
      END
    END AS purchase_category
  FROM customer AS c
  LEFT JOIN customer_demographics AS cd
    ON c.c_current_cdemo_sk = cd.cd_demo_sk
)
SELECT
  dem.c_customer_sk,
  dem.cd_gender,
  dem.purchase_category,
  COALESCE(r.store_return_count, 0) AS store_returns,
  COALESCE(r.web_return_count, 0) AS web_returns,
  r.total_combined_return_amt
FROM CustomerDemographics AS dem
LEFT JOIN CombinedReturns AS r
  ON dem.c_customer_sk = r.customer_sk
WHERE
  (
    dem.cd_gender = 'F' OR dem.cd_gender = 'M'
  )
  AND (
    NOT dem.cd_credit_rating IS NULL AND dem.cd_credit_rating <> 'Bad'
  )
  AND dem.c_customer_sk IN (
    SELECT
      c_customer_sk
    FROM store_sales
    WHERE
      ss_net_paid > (
        SELECT
          AVG(ss_net_paid)
        FROM store_sales
      )
      AND ss_sold_date_sk BETWEEN (
        SELECT
          MIN(d_date_sk)
        FROM date_dim
        WHERE
          d_year = 2021
      ) AND (
        SELECT
          MAX(d_date_sk)
        FROM date_dim
        WHERE
          d_year = 2021
      )
  )
ORDER BY
  r.total_combined_return_amt DESC
LIMIT 100;
