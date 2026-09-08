SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  m.production_year AS release_year,
  c.kind AS company_type,
  STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
  ROW_NUMBER() OVER (PARTITION BY a.id ORDER BY m.production_year DESC) AS movie_rank
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN title AS m
  ON t.id = m.id
WHERE
  a.name ILIKE '%Tom%' AND m.production_year >= 2000
GROUP BY
  a.name,
  t.title,
  m.production_year,
  c.kind,
  a.id
ORDER BY
  actor_name,
  release_year DESC;
