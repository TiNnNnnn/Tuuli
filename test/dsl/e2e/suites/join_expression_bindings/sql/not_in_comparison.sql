SELECT 'empty' AS rhs, l.i FROM binding_left l
WHERE l.i NOT IN (SELECT r.i FROM binding_right r WHERE r.i > 10)
UNION ALL
SELECT 'nonnull', l.i FROM binding_left l
WHERE l.i NOT IN (SELECT r.i FROM binding_right r WHERE r.i = 3)
UNION ALL
SELECT 'nullable', l.i FROM binding_left l
WHERE l.i NOT IN (SELECT r.i FROM binding_right r)
ORDER BY 1, 2 NULLS LAST;
