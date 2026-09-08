SELECT
  t.title AS movie_title,
  a.name AS actor_name,
  p.info AS person_info,
  c.kind AS company_type,
  k.keyword AS movie_keyword,
  r.role AS actor_role
FROM title AS t
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS ci
  ON cc.subject_id = ci.id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
JOIN person_info AS p
  ON a.person_id = p.person_id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_type AS c
  ON mc.company_type_id = c.id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN role_type AS r
  ON ci.role_id = r.id
WHERE
  t.production_year BETWEEN 2000 AND 2023
  AND c.kind IN ('Production', 'Distribution')
  AND p.info_type_id IN (1, 2)
ORDER BY
  t.title,
  a.name;
