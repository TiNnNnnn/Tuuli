SELECT label FROM call_input
WHERE (CASE WHEN i = 0 THEN i ELSE j END) #=# j;
