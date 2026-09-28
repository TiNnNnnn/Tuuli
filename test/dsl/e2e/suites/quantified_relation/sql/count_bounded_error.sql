SELECT i FROM quant_outer
WHERE v::bigint = ANY (SELECT count(1 / x) FROM (VALUES (0), (1)) AS q(x))
ORDER BY i;
