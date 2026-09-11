-- Integer addition can overflow in general, even though this fixture cannot.
-- Keep ErrorFree conservative; this is a rejection control, not an unsafe rule.
SELECT p_partkey, p_size + 1 AS computed_value
FROM part
WHERE p_size > :minimum_size
ORDER BY p_partkey;
