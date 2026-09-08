SELECT
  t.title,
  a.name AS actor_name,
  ci.note AS role_note
FROM title AS t
JOIN movie_companies AS mc
  ON mc.movie_id = t.id
JOIN company_name AS c
  ON c.id = mc.company_id
JOIN complete_cast AS cc
  ON cc.movie_id = t.id
JOIN cast_info AS ci
  ON ci.movie_id = cc.movie_id AND ci.person_id = cc.subject_id
JOIN aka_name AS a
  ON a.person_id = ci.person_id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC;
