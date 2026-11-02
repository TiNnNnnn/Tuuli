SELECT o.k FROM (VALUES ('A'::bpchar), ('A'::bpchar), ('B'::bpchar)) AS o(k) WHERE o.k IN (SELECT pg_catalog.bpchar(i.k, 6, false) FROM (VALUES ('A'::bpchar), ('C'::bpchar)) AS i(k)) ORDER BY o.k;
