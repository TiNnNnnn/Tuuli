SELECT
  t.title,
  a.name,
  c.nr_order,
  cc.kind
FROM title AS t
JOIN cast_info AS c
  ON t.id = c.movie_id
JOIN aka_name AS a
  ON c.person_id = a.person_id
JOIN comp_cast_type AS cc
  ON c.person_role_id = cc.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year,
  c.nr_order;
