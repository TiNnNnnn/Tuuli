-- A total built-in comparison; move eligibility must not depend on fixture values.
SELECT p_partkey, p_size > 25 AS computed_value
FROM part
WHERE p_size > :minimum_size
ORDER BY p_partkey;
