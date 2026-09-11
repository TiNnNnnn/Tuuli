-- Deterministic domain-coverage fixture, NOT dbgen or a sampled population.
-- Fresh TPC-H schema only. Keep tpch_small_data.sql as a separate data batch.
-- Full part/supplier product and cyclic dates/keys intentionally create known
-- correlations; use this for correctness/observability, not population claims.
BEGIN;
INSERT INTO region (r_regionkey, r_name, r_comment) VALUES
  (1, 'EUROPE', 'region Europe'), (2, 'ASIA', 'region Asia'),
  (3, 'AMERICA', 'region America');
INSERT INTO nation (n_nationkey, n_name, n_regionkey, n_comment) VALUES
  (1, 'GERMANY', 1, 'nation Germany'), (2, 'SINGAPORE', 2, 'nation Singapore'),
  (3, 'INDIA', 2, 'nation India'), (4, 'BRAZIL', 3, 'nation Brazil');
INSERT INTO supplier (s_suppkey, s_name, s_address, s_nationkey, s_phone, s_acctbal, s_comment)
SELECT i, 'supplier-' || i, 'address-' || i, 1 + (i - 1) % 4,
       '10-100-100-' || lpad(i::text, 4, '0'), 100 * i,
       repeat('supplier comment ', 4) || i
FROM generate_series(1, 16) AS i;
INSERT INTO part (p_partkey, p_name, p_mfgr, p_brand, p_type, p_size,
                  p_container, p_retailprice, p_comment)
SELECT i, 'part-' || repeat('p', i % 16) || '-' || i,
       'manufacturer-' || (i % 4), 'Brand' || chr(65 + (i - 1) % 4),
       'type-' || (i % 8), 1 + (i - 1) % 50, 'BOX', 100 + i * 7,
       'part comment ' || i
FROM generate_series(1, 128) AS i;
INSERT INTO partsupp (ps_partkey, ps_suppkey, ps_availqty, ps_supplycost, ps_comment)
SELECT p, s, 100 + p + s, 10 + p + s / 10.0, 'supply comment'
FROM generate_series(1, 128) AS p CROSS JOIN generate_series(1, 16) AS s;
INSERT INTO customer (c_custkey, c_name, c_address, c_nationkey, c_phone,
                      c_acctbal, c_mktsegment, c_comment)
SELECT i, 'customer-' || i, 'address-' || i, 1 + (i - 1) % 4,
       '10-200-200-' || lpad(i::text, 4, '0'), 50 * i,
       (ARRAY['BUILDING', 'AUTOMOBILE', 'HOUSEHOLD', 'MACHINERY'])[1 + (i - 1) % 4],
       'customer comment ' || i
FROM generate_series(1, 256) AS i;
INSERT INTO orders (o_orderkey, o_custkey, o_orderstatus, o_totalprice, o_orderdate,
                    o_orderpriority, o_clerk, o_shippriority, o_comment)
SELECT i, 1 + (i - 1) % 256, (ARRAY['O', 'F', 'P'])[1 + (i - 1) % 3],
       1000 + i * 13, make_date(1992 + (i - 1) % 7, 1, 1) + ((i - 1) / 7) % 360,
       (ARRAY['1-URGENT', '2-HIGH', '3-MEDIUM', '4-NOT SPECIFIED', '5-LOW'])[1 + (i - 1) % 5],
       'clerk-' || (i % 8), 0, 'order comment ' || i
FROM generate_series(1, 2048) AS i;
INSERT INTO lineitem (l_orderkey, l_partkey, l_suppkey, l_linenumber, l_quantity,
                      l_extendedprice, l_discount, l_tax, l_returnflag, l_linestatus,
                      l_shipdate, l_commitdate, l_receiptdate, l_shipinstruct, l_shipmode, l_comment)
SELECT o_orderkey, 1 + (o_orderkey + j - 2) % 128, 1 + (o_orderkey + 3 * j - 2) % 16,
       j, 1 + o_orderkey % 50, 1000 + 123.45 * (o_orderkey % 97) + j / 100.0,
       (o_orderkey % 10) / 100.0, (j % 3) / 100.0,
       (ARRAY['N', 'R', 'A'])[j], (ARRAY['O', 'F'])[1 + o_orderkey % 2],
       o_orderdate + 10, o_orderdate + 15, o_orderdate + 12 + o_orderkey % 8,
       'DELIVER IN PERSON', (ARRAY['AIR', 'SHIP', 'RAIL'])[j], 'line comment'
FROM orders CROSS JOIN generate_series(1, 3) AS j;
COMMIT;
ANALYZE;
