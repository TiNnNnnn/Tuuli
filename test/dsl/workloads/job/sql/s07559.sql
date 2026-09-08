SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  t.production_year,
  STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
  ct.kind AS company_type
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
WHERE
  t.production_year >= 2000 AND ct.kind IN ('Distributor', 'Production')
GROUP BY
  a.name,
  t.title,
  t.production_year,
  ct.kind
ORDER BY
  t.production_year DESC,
  a.name;
