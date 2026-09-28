-- A scalar aggregate over empty input returns one NULL row, not zero rows.
SELECT i FROM quant_outer
WHERE v = ALL (SELECT max(v) FROM quant_empty)
ORDER BY i;
