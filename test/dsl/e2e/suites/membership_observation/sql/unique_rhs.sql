SELECT l.k + 0 FROM membership_left l WHERE l.k IN (SELECT r.k FROM membership_right r) ORDER BY 1;
