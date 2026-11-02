SELECT o.k
FROM (VALUES ('A'::varchar(2)), ('A'::varchar(2)), ('B'::varchar(2))) AS o(k)
WHERE o.k IN (
    SELECT i.k
    FROM (VALUES ('A'::varchar(2)), ('C'::varchar(2))) AS i(k))
ORDER BY o.k;
