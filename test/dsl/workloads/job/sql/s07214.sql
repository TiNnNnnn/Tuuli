SELECT
  a.name AS actor_name,
  t.title AS movie_title,
  c.role_id AS role_id,
  ct.kind AS company_type,
  COUNT(DISTINCT m.company_id) AS company_count,
  AVG(CAST(mi.info AS INT)) AS avg_movie_rating
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN aka_title AS t
  ON c.movie_id = t.id
LEFT JOIN movie_companies AS m
  ON t.id = m.movie_id
LEFT JOIN company_type AS ct
  ON m.company_type_id = ct.id
LEFT JOIN movie_info AS mi
  ON t.id = mi.movie_id
  AND mi.info_type_id = (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'rating'
  )
WHERE
  t.production_year BETWEEN 1990 AND 2020
GROUP BY
  a.name,
  t.title,
  c.role_id,
  ct.kind
HAVING
  COUNT(DISTINCT m.company_id) > 1
ORDER BY
  avg_movie_rating DESC,
  actor_name ASC;
