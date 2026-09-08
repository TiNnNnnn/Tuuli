SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.nr_order AS role_order,
  r.role AS role_name
FROM cast_info AS c
JOIN aka_name AS a
  ON c.person_id = a.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN role_type AS r
  ON c.role_id = r.id
WHERE
  t.production_year = 2020
ORDER BY
  a.name,
  c.nr_order;
