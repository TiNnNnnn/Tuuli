-- The nested plan raises division_by_zero while searching for the skipped
-- rows. Collapsing to LIMIT 0 would silently remove that error.
SELECT * FROM (
  SELECT * FROM slice_input WHERE 1 / (v - v) > 0 LIMIT 5 OFFSET 2
) AS input LIMIT 4 OFFSET 5;
