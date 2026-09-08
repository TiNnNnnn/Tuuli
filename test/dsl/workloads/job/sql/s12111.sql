SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  p.name AS person_name,
  c.kind AS company_type,
  k.keyword AS movie_keyword,
  r.role AS person_role
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN role_type AS r
  ON ci.role_id = r.id
JOIN name AS p
  ON a.person_id = p.imdb_id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  a.name;
