-- SQL NULL means no count bound, not the natural number zero.
SELECT * FROM (SELECT * FROM slice_input LIMIT NULL OFFSET 2) AS input
LIMIT 4 OFFSET 3;
