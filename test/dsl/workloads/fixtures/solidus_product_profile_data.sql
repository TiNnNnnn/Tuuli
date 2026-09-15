-- Nonempty product/variant/price domain on the original nullable Solidus schema.
-- No WeTune inferred schema patches: deleted_at/country_iso must allow NULL.
-- Preserve duplicate join outputs, absent prices, soft deletion and currency filters.
BEGIN;
INSERT INTO spree_products (id, name, slug, available_on, deleted_at)
SELECT i, CASE WHEN i % 7 = 0 THEN 'fritos-' ELSE 'product-' END || i,
       'product-' || i,
       CASE WHEN i % 17 = 0 THEN TIMESTAMP '2021-01-01' ELSE TIMESTAMP '2020-01-01' END,
       CASE WHEN i % 11 = 0 THEN TIMESTAMP '2020-02-01' END
FROM generate_series(1, 96) AS g(i);

INSERT INTO spree_variants (id, sku, product_id, is_master, deleted_at, position, track_inventory)
SELECT (p.id - 1) * 2 + slot, 'sku-' || p.id || '-' || slot, p.id,
       CASE WHEN slot = 1 OR p.id % 7 = 0 THEN 1 ELSE 0 END,
       CASE WHEN slot = 2 AND p.id % 5 = 0 THEN TIMESTAMP '2020-02-01' END, slot, 1
FROM spree_products p CROSS JOIN generate_series(1, 2) AS g(slot);

INSERT INTO spree_prices (id, variant_id, amount, currency, country_iso, deleted_at)
SELECT (v.id - 1) * 3 + slot, v.id, 10 + v.product_id + slot / 10.0,
       CASE WHEN slot = 3 THEN 'EUR' ELSE 'USD' END,
       CASE WHEN slot = 2 THEN 'US' END,
       CASE WHEN slot = 3 AND v.id % 5 = 0 THEN TIMESTAMP '2020-02-01' END
FROM spree_variants v CROSS JOIN generate_series(1, 3) AS g(slot)
WHERE v.product_id % 13 <> 0;

INSERT INTO spree_taxons (id, name, position)
VALUES (147, 'category-a', 1), (148, 'category-b', 2);
INSERT INTO spree_products_taxons (id, product_id, taxon_id, position)
SELECT id, id, 147 + id % 2, id FROM spree_products;
COMMIT;
ANALYZE;
