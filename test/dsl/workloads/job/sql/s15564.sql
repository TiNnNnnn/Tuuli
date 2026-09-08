SELECT
  t.title,
  a.name,
  c.note
FROM title AS t
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN cast_info AS c
  ON t.id = c.movie_id
JOIN aka_name AS a
  ON c.person_id = a.person_id
WHERE
  t.production_year = 2023
ORDER BY
  t.title,
  a.name;
