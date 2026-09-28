SELECT i FROM quant_outer
WHERE v::bigint > ALL (SELECT count(*) FROM (VALUES (1), (NULL::int)) AS q(x) WHERE x < 0)
ORDER BY i;
