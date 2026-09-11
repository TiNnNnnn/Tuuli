-- Derived from tpch/s13849; bind predicate values, not SQL identifiers.
SELECT p.p_name, SUM(l.l_extendedprice * (1 - l.l_discount)) AS total_revenue
FROM part AS p
JOIN lineitem AS l ON p.p_partkey = l.l_partkey
JOIN orders AS o ON l.l_orderkey = o.o_orderkey
WHERE o.o_orderdate >= :start_date AND o.o_orderdate < :end_date
GROUP BY p.p_name
ORDER BY total_revenue DESC
LIMIT 10;
