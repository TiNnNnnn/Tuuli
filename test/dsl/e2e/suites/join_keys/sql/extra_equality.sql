SELECT DISTINCT c.k FROM key_child c JOIN key_parent p
ON c.k = p.k AND c.extra = p.extra ORDER BY c.k;
