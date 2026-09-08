SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  ci.note AS role_note,
  ci.nr_order AS role_order,
  c.name AS company_name,
  ct.kind AS company_type,
  mi.info AS movie_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN movie_info AS mi
  ON t.movie_id = mi.movie_id
JOIN movie_companies AS mc
  ON t.movie_id = mc.movie_id
JOIN company_name AS c
  ON mc.company_id = c.id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
LEFT JOIN movie_keyword AS mk
  ON t.movie_id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000
  AND ci.nr_order < 5
  AND mi.info_type_id IN (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Box Office'
  )
ORDER BY
  t.production_year DESC,
  a.name,
  t.title;
