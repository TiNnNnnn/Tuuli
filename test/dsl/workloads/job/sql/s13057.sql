SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  n.name AS person_name,
  rt.role AS role,
  ci.kind AS comp_cast_type
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN name AS n
  ON a.person_id = n.imdb_id
JOIN role_type AS rt
  ON c.role_id = rt.id
JOIN comp_cast_type AS ci
  ON c.person_role_id = ci.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  a.name;
