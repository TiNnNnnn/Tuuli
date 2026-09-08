SELECT
  t.title AS movie_title,
  a.name AS actor_name,
  ct.kind AS company_type,
  k.keyword AS movie_keyword,
  COUNT(*) OVER (PARTITION BY t.id) AS total_cast
FROM title AS t
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS ci
  ON cc.subject_id = ci.id
JOIN aka_name AS a
  ON ci.person_id = a.person_id
LEFT JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
WHERE
  t.production_year >= 2000 AND NOT a.name IS NULL
GROUP BY
  t.title,
  a.name,
  ct.kind,
  k.keyword,
  t.id
ORDER BY
  t.production_year DESC,
  total_cast DESC;
