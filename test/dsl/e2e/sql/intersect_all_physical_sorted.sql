-- bit has ordering/equality but no hash operator class.
SELECT COALESCE(a::integer, -1), COALESCE(b::integer, -1) FROM (
    SELECT a::bit(32), b::bit(32) FROM dsl_bag_pair_left
    INTERSECT ALL
    SELECT a::bit(32), b::bit(32) FROM dsl_bag_pair_right
) s ORDER BY 1, 2;
