SELECT
  t.title AS movie_title,
  a.name AS actor_name,
  ci.nr_order AS actor_order,
  ct.kind AS company_type,
  COUNT(mk.keyword_id) AS keyword_count
FROM title AS t
JOIN complete_cast AS cc
  ON t.id = cc.movie_id
JOIN cast_info AS ci
  ON ci.id = cc.subject_id
JOIN aka_name AS a
  ON a.person_id = ci.person_id
JOIN movie_companies AS mc
  ON mc.movie_id = t.id
JOIN company_type AS ct
  ON ct.id = mc.company_type_id
JOIN movie_keyword AS mk
  ON mk.movie_id = t.id
GROUP BY
  t.title,
  a.name,
  ci.nr_order,
  ct.kind
ORDER BY
  t.title,
  actor_order;
