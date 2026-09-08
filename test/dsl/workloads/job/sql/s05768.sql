SELECT
  a.name AS actor_name,
  m.title AS movie_title,
  m.production_year,
  STRING_AGG(k.keyword, ', ') AS keywords,
  c.kind AS company_type
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS m
  ON ci.movie_id = m.id
JOIN movie_companies AS mc
  ON m.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
LEFT JOIN movie_keyword AS mk
  ON m.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  m.production_year >= 2000 AND NOT c.kind IS NULL
GROUP BY
  a.name,
  m.title,
  m.production_year,
  c.kind
ORDER BY
  m.production_year DESC,
  a.name ASC;
