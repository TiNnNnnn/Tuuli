/* end query 13 in stream 0 using template query91.tpl */ /* start query 14 in stream 0 using template query21.tpl */
SELECT
  *
FROM (
  SELECT
    w_warehouse_name,
    i_item_id,
    SUM(
      CASE
        WHEN (
          CAST(d_date AS DATE) < CAST('1999-05-02' AS DATE)
        )
        THEN inv_quantity_on_hand
        ELSE 0
      END
    ) AS inv_before,
    SUM(
      CASE
        WHEN (
          CAST(d_date AS DATE) >= CAST('1999-05-02' AS DATE)
        )
        THEN inv_quantity_on_hand
        ELSE 0
      END
    ) AS inv_after
  FROM inventory, warehouse, item, date_dim
  WHERE
    i_current_price BETWEEN 0.99 AND 1.49
    AND i_item_sk = inv_item_sk
    AND inv_warehouse_sk = w_warehouse_sk
    AND inv_date_sk = d_date_sk
    AND d_date BETWEEN (
      CAST('1999-05-02' AS DATE) - INTERVAL '30 DAY'
    ) AND (
      CAST('1999-05-02' AS DATE) + INTERVAL '30 DAY'
    )
  GROUP BY
    w_warehouse_name,
    i_item_id
) AS x
WHERE
  (
    CASE WHEN inv_before > 0 THEN inv_after / inv_before ELSE NULL END
  ) BETWEEN 2.0 / 3.0 AND 3.0 / 2.0
ORDER BY
  w_warehouse_name,
  i_item_id
LIMIT 100;
