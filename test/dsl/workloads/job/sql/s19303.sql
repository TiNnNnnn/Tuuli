SELECT
  t.title,
  a.name AS actor_name,
  c.nr_order,
  ct.kind AS company_type
FROM title AS t
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
JOIN cast_info AS c
  ON t.id = c.movie_id
JOIN aka_name AS a
  ON c.person_id = a.person_id
WHERE
  t.production_year = 2022
ORDER BY
  t.title,
  c.nr_order;
