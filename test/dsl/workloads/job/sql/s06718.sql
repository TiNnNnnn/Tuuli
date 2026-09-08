SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  ci.nr_order AS role_order,
  ct.kind AS cast_type,
  cn.name AS company_name,
  mi.info AS movie_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN kind_type AS kt
  ON t.kind_id = kt.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
JOIN movie_info AS mi
  ON t.id = mi.movie_id
JOIN info_type AS it
  ON mi.info_type_id = it.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000 AND ct.kind = 'Distributor'
ORDER BY
  t.production_year DESC,
  a.name ASC;
