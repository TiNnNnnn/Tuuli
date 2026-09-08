SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  comp.name AS company_name,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN aka_title AS t
  ON c.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS comp
  ON mc.company_id = comp.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year > 2000
ORDER BY
  t.production_year DESC,
  a.name;
