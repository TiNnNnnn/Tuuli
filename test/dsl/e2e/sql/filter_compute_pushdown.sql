-- Preserve the varchar-to-text Compute context of Calcite query 175.
SELECT label AS key
FROM dsl_compute_group
WHERE label > 'b'
GROUP BY label
HAVING label > 'c' AND (COUNT(*) > 30 OR label < 'z');
