SELECT i, CASE WHEN i > 1 THEN v ELSE 'none'::varchar(8) END FROM column_values ORDER BY i;
