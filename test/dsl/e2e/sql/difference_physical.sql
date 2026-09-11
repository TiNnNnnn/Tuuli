-- No optional rewrite may be needed to execute native set semantics.
SELECT COALESCE(a, -1), COALESCE(b, -1) FROM (
    SELECT a, b FROM dsl_bag_pair_left
    EXCEPT
    SELECT a, b FROM dsl_bag_pair_right
) s ORDER BY 1, 2;
