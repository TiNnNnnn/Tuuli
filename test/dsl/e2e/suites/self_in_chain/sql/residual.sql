SELECT id, v FROM chain_input o
WHERE id IN (SELECT id FROM chain_input i) AND v > 10
ORDER BY id;
