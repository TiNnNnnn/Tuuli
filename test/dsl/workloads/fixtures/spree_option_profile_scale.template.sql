-- Synthetic scale probe; unique positions make the original ORDER BY deterministic.
INSERT INTO spree_option_types (id, name, presentation, position, created_at, updated_at)
VALUES (5, 'size', 'Size', 5, TIMESTAMP '2020-01-01', TIMESTAMP '2020-01-01');
INSERT INTO spree_option_values (id, position, name, presentation, option_type_id,
                                 created_at, updated_at)
SELECT i, i, CASE WHEN i % 10 < 3 THEN 'Size-5-' || i ELSE 'Other-' || i END,
       'value_' || i, 5, TIMESTAMP '2020-01-01', TIMESTAMP '2020-01-01'
FROM generate_series(1, ${rows}) AS g(i);
ANALYZE;
