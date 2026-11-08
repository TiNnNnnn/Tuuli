SELECT l.k + 0 FROM membership_left l WHERE l.k IN (SELECT r.k FROM membership_duplicates r) ORDER BY 1;
