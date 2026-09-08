SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  t.production_year,
  c.kind AS cast_kind,
  STRING_AGG(DISTINCT kw.keyword, ', ') AS keywords,
  STRING_AGG(DISTINCT co.name, ', ') AS companies
FROM aka_name AS a
JOIN cast_info AS ci
  ON a.person_id = ci.person_id
JOIN title AS t
  ON ci.movie_id = t.id
JOIN comp_cast_type AS c
  ON ci.person_role_id = c.id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS kw
  ON mk.keyword_id = kw.id
LEFT JOIN movie_companies AS mc
  ON t.id = mc.movie_id
LEFT JOIN company_name AS co
  ON mc.company_id = co.id
WHERE
  t.production_year >= 2000 AND NOT a.name IS NULL
GROUP BY
  a.name,
  t.title,
  t.production_year,
  c.kind
ORDER BY
  t.production_year DESC,
  a.name;
