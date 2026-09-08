SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  p.info AS person_info,
  k.keyword AS movie_keyword,
  cn.name AS company_name
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN aka_title AS t
  ON c.movie_id = t.movie_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN company_name AS cn
  ON cc.subject_id = cn.imdb_id
JOIN name AS n
  ON a.person_id = n.imdb_id
JOIN person_info AS p
  ON a.person_id = p.person_id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  a.name;
