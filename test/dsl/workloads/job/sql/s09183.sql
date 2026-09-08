SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.kind AS cast_type,
  p.info AS person_info,
  k.keyword AS movie_keyword
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN aka_title AS t
  ON ci.movie_id = t.movie_id
JOIN comp_cast_type AS c
  ON ci.person_role_id = c.id
JOIN person_info AS p
  ON a.person_id = p.person_id
JOIN movie_keyword AS mk
  ON t.movie_id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year BETWEEN 1990 AND 2000
  AND c.kind = 'actor'
  AND p.info_type_id IN (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'birth date'
  )
ORDER BY
  t.production_year DESC,
  a.name;
