-- Plain Limit templates must not consume the inner ordering requirement.
SELECT * FROM (SELECT * FROM slice_input ORDER BY v LIMIT 5 OFFSET 2) AS input
LIMIT 4 OFFSET 3;
