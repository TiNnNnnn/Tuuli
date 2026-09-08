SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  t.production_year,
  c.kind AS company_type,
  m.info AS movie_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_info AS m
  ON t.id = m.movie_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year BETWEEN 2000 AND 2020 AND k.keyword LIKE 'Action%'
ORDER BY
  t.production_year DESC,
  a.name;
