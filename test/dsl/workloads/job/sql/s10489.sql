SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  ri.role AS person_role,
  m.name AS company_name
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS m
  ON mc.company_id = m.id
JOIN role_type AS ri
  ON c.role_id = ri.id
WHERE
  t.production_year = 2022
ORDER BY
  t.title,
  a.name;
