-- PG supplies a default RANGE frame even for row_number; retain that carrier.
SELECT i, row_number() OVER (PARTITION BY i ORDER BY j)
FROM binding_input WHERE i > 0;
