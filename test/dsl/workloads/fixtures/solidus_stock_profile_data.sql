-- Synthetic inventory domain with duplicate projected keys, NULL locations,
-- inactive locations and deleted/non-backorderable items. Original schema kept.
INSERT INTO spree_stock_locations (id, name, "default", active, restock_inventory, fulfillable)
SELECT i, 'location_' || i, 0, CASE WHEN i % 3 = 0 THEN 0 ELSE 1 END, 0, 1
FROM generate_series(1, 32) AS g(i);
INSERT INTO spree_stock_items (id, stock_location_id, variant_id, count_on_hand,
                              backorderable, deleted_at)
SELECT i, CASE WHEN i % 11 = 0 THEN NULL ELSE 1 + i % 32 END,
       CASE WHEN i % 3 = 0 THEN 990 ELSE 991 END, i % 20, i % 2,
       CASE WHEN i % 7 = 0 THEN TIMESTAMP '2020-01-01' END
FROM generate_series(1, 1024) AS g(i);
ANALYZE;
