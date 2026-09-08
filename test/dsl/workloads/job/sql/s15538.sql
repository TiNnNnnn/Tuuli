SELECT
  t.title AS movie_title,
  a.name AS actor_name,
  ci.note AS role_note
FROM title AS t
JOIN cast_info AS ci
  ON t.id = ci.movie_id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
WHERE
  t.production_year >= 2000
ORDER BY
  t.production_year DESC,
  a.name;
