SELECT
  t.title AS movie_title,
  t.production_year,
  a.name AS actor_name,
  r.role AS actor_role,
  c.name AS company_name,
  k.keyword AS movie_keyword
FROM title AS t
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS c
  ON mc.company_id = c.id
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS ci
  ON cc.subject_id = ci.id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
JOIN role_type AS r
  ON ci.role_id = r.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year,
  t.title;
