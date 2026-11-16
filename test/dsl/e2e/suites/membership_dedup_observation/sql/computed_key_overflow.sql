SELECT l.k FROM membership_left l WHERE l.k IN (SELECT r.k * 2147483647 FROM membership_duplicates r) ORDER BY 1;
