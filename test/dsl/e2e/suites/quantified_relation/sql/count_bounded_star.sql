SELECT i FROM quant_outer
WHERE v::bigint = ANY (SELECT count(*) FROM (VALUES (1), (NULL::int)) AS q(x))
ORDER BY i;
