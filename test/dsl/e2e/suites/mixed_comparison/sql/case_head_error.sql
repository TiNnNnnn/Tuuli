SELECT i FROM comparison_error
WHERE (CASE WHEN i = 0 THEN i ELSE j END) #=# j;
