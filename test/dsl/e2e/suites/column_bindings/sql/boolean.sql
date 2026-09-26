SELECT i, CASE WHEN i > 1 THEN b ELSE false END FROM column_values ORDER BY i;
