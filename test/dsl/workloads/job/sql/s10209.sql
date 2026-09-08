SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  ci.note AS cast_note,
  cn.name AS company_name,
  ki.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS ki
  ON mk.keyword_id = ki.id
WHERE
  t.production_year > 2000
ORDER BY
  t.production_year DESC,
  a.name,
  t.title;
