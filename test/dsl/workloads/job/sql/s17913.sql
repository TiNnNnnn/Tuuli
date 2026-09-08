SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  t.production_year,
  ci.nr_order AS cast_order
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  ci.nr_order;
