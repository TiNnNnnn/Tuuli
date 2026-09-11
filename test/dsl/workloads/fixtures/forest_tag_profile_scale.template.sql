-- Fixed-width values; the original primary-key predicate always returns one row.
INSERT INTO tag (tag_id, tag_name, tag_description)
SELECT i, 'tag_' || lpad(i::text, 8, '0'), repeat('x', 64)
FROM generate_series(1, ${rows}) AS g(i);
ANALYZE;
