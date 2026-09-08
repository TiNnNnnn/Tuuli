SELECT
  a.name AS actor_name,
  m.title AS movie_title,
  m.production_year,
  c.kind AS cast_type,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS m
  ON ci.movie_id = m.id
JOIN comp_cast_type AS c
  ON ci.person_role_id = c.id
JOIN movie_keyword AS mk
  ON m.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  m.production_year > 2000
ORDER BY
  m.production_year DESC,
  a.name;
