SELECT l.k + 0, l.v
FROM membership_left l
WHERE EXISTS (
    SELECT 1 FROM membership_pairs r
    WHERE r.k = l.k AND r.v = l.v)
ORDER BY 1,2;
