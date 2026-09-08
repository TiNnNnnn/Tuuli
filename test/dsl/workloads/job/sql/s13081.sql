SELECT
  t.title,
  a.name AS actor_name,
  c.kind AS cast_type,
  mt.info AS movie_info,
  k.keyword
FROM aka_title AS t
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS ci
  ON cc.subject_id = ci.id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
JOIN comp_cast_type AS c
  ON ci.person_role_id = c.id
LEFT JOIN movie_info AS mt
  ON t.id = mt.movie_id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  a.name;
