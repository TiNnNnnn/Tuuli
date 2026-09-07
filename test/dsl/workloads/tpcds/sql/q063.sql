/* end query 61 in stream 0 using template query42.tpl */ /* start query 62 in stream 0 using template query41.tpl */
SELECT DISTINCT
  (
    i_product_name
  )
FROM item AS i1
WHERE
  i_manufact_id BETWEEN 929 AND 929 + 40
  AND (
    SELECT
      COUNT(*) AS item_cnt
    FROM item
    WHERE
      (
        i_manufact = i1.i_manufact
        AND (
          (
            i_category = 'Women'
            AND (
              i_color = 'purple' OR i_color = 'deep'
            )
            AND (
              i_units = 'Gross' OR i_units = 'Gram'
            )
            AND (
              i_size = 'petite' OR i_size = 'small'
            )
          )
          OR (
            i_category = 'Women'
            AND (
              i_color = 'blush' OR i_color = 'cyan'
            )
            AND (
              i_units = 'Dram' OR i_units = 'Carton'
            )
            AND (
              i_size = 'extra large' OR i_size = 'N/A'
            )
          )
          OR (
            i_category = 'Men'
            AND (
              i_color = 'almond' OR i_color = 'hot'
            )
            AND (
              i_units = 'Unknown' OR i_units = 'Tbl'
            )
            AND (
              i_size = 'large' OR i_size = 'economy'
            )
          )
          OR (
            i_category = 'Men'
            AND (
              i_color = 'pale' OR i_color = 'burlywood'
            )
            AND (
              i_units = 'Ounce' OR i_units = 'Case'
            )
            AND (
              i_size = 'petite' OR i_size = 'small'
            )
          )
        )
      )
      OR (
        i_manufact = i1.i_manufact
        AND (
          (
            i_category = 'Women'
            AND (
              i_color = 'cornflower' OR i_color = 'drab'
            )
            AND (
              i_units = 'Dozen' OR i_units = 'Tsp'
            )
            AND (
              i_size = 'petite' OR i_size = 'small'
            )
          )
          OR (
            i_category = 'Women'
            AND (
              i_color = 'thistle' OR i_color = 'grey'
            )
            AND (
              i_units = 'Each' OR i_units = 'N/A'
            )
            AND (
              i_size = 'extra large' OR i_size = 'N/A'
            )
          )
          OR (
            i_category = 'Men'
            AND (
              i_color = 'khaki' OR i_color = 'magenta'
            )
            AND (
              i_units = 'Bundle' OR i_units = 'Oz'
            )
            AND (
              i_size = 'large' OR i_size = 'economy'
            )
          )
          OR (
            i_category = 'Men'
            AND (
              i_color = 'dark' OR i_color = 'cornsilk'
            )
            AND (
              i_units = 'Bunch' OR i_units = 'Box'
            )
            AND (
              i_size = 'petite' OR i_size = 'small'
            )
          )
        )
      )
  ) > 0
ORDER BY
  i_product_name
LIMIT 100;
