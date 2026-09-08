SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.kind AS company_type
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
WHERE
  t.production_year = 2020;
