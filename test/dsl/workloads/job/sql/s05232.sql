SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.kind AS cast_type,
  co.name AS company_name,
  k.keyword AS movie_keyword,
  mi.info AS movie_info,
  COUNT(*) AS total_contributions
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS co
  ON mc.company_id = co.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
LEFT JOIN movie_info AS mi
  ON t.id = mi.movie_id
LEFT JOIN comp_cast_type AS c
  ON ci.person_role_id = c.id
WHERE
  t.production_year > 2000 AND co.country_code = 'USA'
GROUP BY
  a.name,
  t.title,
  c.kind,
  co.name,
  k.keyword,
  mi.info
ORDER BY
  total_contributions DESC,
  t.title ASC
LIMIT 100;
