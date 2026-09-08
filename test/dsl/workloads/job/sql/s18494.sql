SELECT
  t.title,
  a.name,
  c.nr_order,
  r.role
FROM title AS t
JOIN cast_info AS c
  ON t.id = c.movie_id
JOIN aka_name AS a
  ON c.person_id = a.person_id
JOIN role_type AS r
  ON c.role_id = r.id
WHERE
  t.production_year = 2023
ORDER BY
  t.title;
