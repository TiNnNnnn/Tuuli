SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.kind AS company_type,
  k.keyword AS movie_keyword,
  p.info AS person_info
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN person_info AS p
  ON a.person_id = p.person_id
WHERE
  t.production_year > 2000
  AND p.info_type_id IN (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Biography'
  )
ORDER BY
  t.production_year DESC,
  a.name;
