SELECT
  t.title,
  a.name AS actor_name,
  c.kind AS comp_cast_type,
  m.name AS company_name,
  k.keyword,
  i.info
FROM title AS t
JOIN cast_info AS ci
  ON t.id = ci.movie_id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
JOIN comp_cast_type AS c
  ON ci.role_id = c.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS m
  ON mc.company_id = m.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN movie_info AS mi
  ON t.id = mi.movie_id
JOIN info_type AS i
  ON mi.info_type_id = i.id
WHERE
  t.production_year >= 2000 AND m.country_code = 'USA'
ORDER BY
  t.title,
  a.name;
