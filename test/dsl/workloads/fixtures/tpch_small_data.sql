-- Deterministic populated fixture, NOT a TPC-H dbgen scale or performance benchmark.
-- Load only into the runner's fresh TPC-H schema, with all foreign keys intact.
INSERT INTO region (r_regionkey, r_name) VALUES (1, 'EUROPE');
INSERT INTO nation (n_nationkey, n_name, n_regionkey) VALUES (1, 'GERMANY', 1);
INSERT INTO supplier (s_suppkey, s_name, s_nationkey) VALUES (1, 'supplier', 1);
INSERT INTO part (p_partkey, p_name) VALUES (1, 'part');
INSERT INTO partsupp (ps_partkey, ps_suppkey, ps_availqty, ps_supplycost) VALUES (1, 1, 100, 10);
INSERT INTO customer (c_custkey, c_name, c_nationkey, c_mktsegment)
SELECT i, 'customer-' || i, 1, 'BUILDING' FROM generate_series(1, 100) AS i;
INSERT INTO orders (o_orderkey, o_custkey, o_orderstatus, o_totalprice, o_orderdate,
                    o_orderpriority, o_clerk, o_shippriority)
SELECT i, 1 + i % 100, 'O', 100 + i % 1000, DATE '1994-01-01' + (i % 500),
       'priority-' || (i % 5), 'clerk', 0
FROM generate_series(1, 2000) AS i;
INSERT INTO lineitem (l_orderkey, l_partkey, l_suppkey, l_linenumber, l_quantity,
                      l_extendedprice, l_discount, l_tax, l_shipdate, l_commitdate, l_receiptdate)
SELECT i, 1, 1, j, 1 + i % 10, 100 + i % 1000, 0.05, 0.02,
       DATE '1995-04-01', DATE '1995-04-02', DATE '1995-04-01' + (i % 4)
FROM generate_series(1, 2000) AS i CROSS JOIN generate_series(1, 3) AS j;
ANALYZE;
