SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  p.name AS person_name,
  c.kind AS company_type,
  kw.keyword AS movie_keyword
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
JOIN keyword AS kw
  ON mk.keyword_id = kw.id
JOIN name AS p
  ON a.person_id = p.imdb_id
WHERE
  t.production_year BETWEEN 1990 AND 2020
ORDER BY
  t.production_year,
  a.name;
