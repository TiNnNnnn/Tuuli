SELECT scenario, value FROM (
    SELECT 'any_count' AS scenario, o.case_id AS value FROM dsl_notin_outer o
    WHERE o.x::bigint = ANY (
        SELECT count(DISTINCT v.x) FROM (VALUES (1), (1), (2), (NULL::int)) v(x)
        WHERE v.x = o.x)
    UNION ALL
    SELECT 'all_count', o.case_id FROM dsl_notin_outer o
    WHERE o.x::bigint < ALL (
        SELECT count(DISTINCT v.x) FROM (VALUES (1), (1), (2), (NULL::int)) v(x))
    UNION ALL
    SELECT 'empty_count', o.case_id FROM dsl_notin_outer o
    WHERE 0::bigint = ALL (
        SELECT count(DISTINCT v.x) FROM (VALUES (1), (NULL::int)) v(x) WHERE false)
) cases
ORDER BY scenario, value;
