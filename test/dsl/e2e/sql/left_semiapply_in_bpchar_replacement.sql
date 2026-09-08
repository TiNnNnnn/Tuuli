SELECT o.k
FROM (VALUES ('A'::char(2)), ('A'::char(2)), ('B'::char(2))) AS o(k)
WHERE o.k IN (
    SELECT i.k
    FROM (VALUES ('A'::char(2)), ('C'::char(2))) AS i(k))
ORDER BY o.k;
