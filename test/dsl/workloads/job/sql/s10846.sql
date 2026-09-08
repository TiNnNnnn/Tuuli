SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  t.production_year,
  r.role AS actor_role,
  c.kind AS company_type
FROM cast_info AS ci
JOIN aka_name AS a
  ON ci.person_id = a.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN role_type AS r
  ON ci.role_id = r.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC;
