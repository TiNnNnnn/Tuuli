SELECT id, v FROM chain_duplicates o
WHERE id IN (SELECT id FROM chain_duplicates i) AND v > 10
ORDER BY id, v;
