SELECT
  t.title AS movie_title,
  a.name AS actor_name,
  r.role AS role_name,
  c.note AS casting_note,
  cm.name AS company_name,
  kt.keyword AS movie_keyword,
  mi.info AS movie_info
FROM aka_title AS t
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS c
  ON cc.subject_id = c.id
JOIN aka_name AS a
  ON c.person_id = a.person_id
JOIN role_type AS r
  ON c.role_id = r.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cm
  ON mc.company_id = cm.id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS kt
  ON mk.keyword_id = kt.id
LEFT JOIN movie_info AS mi
  ON t.id = mi.movie_id
WHERE
  t.production_year > 2000 AND a.name LIKE 'J%'
ORDER BY
  t.title ASC,
  a.name ASC;
