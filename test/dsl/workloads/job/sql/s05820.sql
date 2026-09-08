SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  co.name AS company_name,
  cct.kind AS company_type,
  mt.info AS movie_info
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS co
  ON mc.company_id = co.id
JOIN company_type AS cct
  ON mc.company_type_id = cct.id
JOIN movie_info AS mt
  ON t.id = mt.movie_id
WHERE
  t.production_year BETWEEN 2000 AND 2020 AND cct.kind LIKE '%Production%'
ORDER BY
  t.production_year DESC,
  a.name,
  t.title;
