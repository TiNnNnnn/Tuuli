SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  t.production_year,
  c.role_id,
  rc.role AS role_name,
  cm.name AS company_name,
  it.info AS movie_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN role_type AS rc
  ON c.role_id = rc.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cm
  ON mc.company_id = cm.id
LEFT JOIN movie_info AS mi
  ON t.id = mi.movie_id
LEFT JOIN info_type AS it
  ON mi.info_type_id = it.id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year BETWEEN 2000 AND 2020
  AND NOT a.name IS NULL
  AND mc.company_type_id = (
    SELECT
      id
    FROM company_type
    WHERE
      kind = 'Distributor'
    LIMIT 1
  )
ORDER BY
  t.production_year DESC,
  a.name,
  t.title;
