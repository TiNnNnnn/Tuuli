SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.nr_order AS cast_order,
  n.name AS person_name,
  p.info AS person_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN name AS n
  ON c.person_id = n.imdb_id
JOIN person_info AS p
  ON n.id = p.person_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  c.nr_order;
