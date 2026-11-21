SELECT 'nonempty', l.i FROM binding_left l
WHERE NOT EXISTS (SELECT 1 FROM binding_right r WHERE l.i > 1)
UNION ALL
SELECT 'empty', l.i FROM binding_left l
WHERE NOT EXISTS (SELECT 1 FROM binding_right r WHERE r.i > 100 AND l.i > 1)
ORDER BY 1, 2 NULLS LAST;
