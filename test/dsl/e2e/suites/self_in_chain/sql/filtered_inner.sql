SELECT id, v FROM chain_input o
WHERE id IN (SELECT id FROM chain_input i WHERE v < 30) AND v > 10
ORDER BY id;
