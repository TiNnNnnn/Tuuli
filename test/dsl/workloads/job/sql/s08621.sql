SELECT
  a.id AS aka_id,
  a.name AS aka_name,
  t.title AS movie_title,
  t.production_year,
  p.info AS person_info,
  k.keyword AS movie_keyword,
  c.name AS company_name,
  rt.role AS person_role
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN person_info AS p
  ON a.person_id = p.person_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS c
  ON mc.company_id = c.id
JOIN role_type AS rt
  ON ci.role_id = rt.id
WHERE
  t.production_year BETWEEN 2000 AND 2023
  AND a.name LIKE 'A%'
  AND c.country_code = 'USA'
ORDER BY
  t.production_year DESC,
  a.name ASC;
