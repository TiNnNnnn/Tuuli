SELECT i FROM quant_outer
WHERE v::bigint = ANY (SELECT count(*) FROM quant_inner GROUP BY v)
ORDER BY i;
