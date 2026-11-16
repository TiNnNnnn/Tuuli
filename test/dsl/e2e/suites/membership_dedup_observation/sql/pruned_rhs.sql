SELECT l.k FROM membership_left l WHERE l.k IN (SELECT r.k FROM membership_pairs r) ORDER BY 1;
