SELECT
  t.title,
  p.name AS person_name,
  c.role_id
FROM title AS t
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS c
  ON cc.subject_id = c.id
JOIN aka_name AS p
  ON c.person_id = p.person_id
WHERE
  t.production_year = 2023
ORDER BY
  t.title,
  p.name;
