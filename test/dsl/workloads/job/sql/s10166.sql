SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  c.nr_order AS cast_order,
  n.name AS person_name,
  rt.role AS role,
  m.info AS movie_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN name AS n
  ON a.person_id = n.imdb_id
JOIN role_type AS rt
  ON c.role_id = rt.id
JOIN movie_info AS m
  ON t.id = m.movie_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year = 2020
ORDER BY
  t.title,
  c.nr_order;
