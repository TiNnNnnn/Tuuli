SELECT o.i, CASE WHEN o.i > 0 THEN (SELECT s.v FROM scalar_text s WHERE s.i = o.i) ELSE 'fallback'::varchar(8) END FROM scalar_outer o ORDER BY o.i;
