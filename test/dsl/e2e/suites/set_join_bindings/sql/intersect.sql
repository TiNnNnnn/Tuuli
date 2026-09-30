-- Opposite physical column order, duplicates and NULLs on either side.
SELECT COALESCE(a,-1), COALESCE(b,-1) FROM (
  SELECT a,b FROM set_left
  INTERSECT
  SELECT b,a FROM set_right
) s ORDER BY 1,2;
