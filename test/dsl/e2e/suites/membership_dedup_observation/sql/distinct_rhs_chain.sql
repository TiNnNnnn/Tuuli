SELECT l.k, l.v
FROM membership_left l
WHERE l.k IN (SELECT r.k FROM membership_duplicates r)
  AND l.k IN (SELECT r.k FROM membership_other r)
ORDER BY 1, 2;
