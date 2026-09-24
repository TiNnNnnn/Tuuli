SELECT o.id FROM equality_input o
WHERE o.id IN (SELECT l.id FROM equality_input l WHERE l.id <= 2 OFFSET 0)
  AND o.id IN (SELECT r.id FROM equality_input r WHERE r.id >= 2 OFFSET 0)
ORDER BY o.id;
