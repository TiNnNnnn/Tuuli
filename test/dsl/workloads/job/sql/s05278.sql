SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.kind AS company_type,
  ti.info AS movie_info,
  STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_info AS mi
  ON t.id = mi.movie_id
JOIN info_type AS ti
  ON mi.info_type_id = ti.id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year > 2000 AND c.kind LIKE 'Production%'
GROUP BY
  a.name,
  t.title,
  c.kind,
  ti.info
ORDER BY
  actor_name,
  movie_title;
