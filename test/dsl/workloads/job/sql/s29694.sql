WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    k.keyword,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS year_rank
  FROM title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    t.production_year > 2000
), ActorTitles AS (
  SELECT
    a.name AS actor_name,
    t.title AS movie_title,
    t.production_year,
    COUNT(DISTINCT cc.id) AS total_cast_roles
  FROM aka_name AS a
  JOIN cast_info AS ci
    ON a.person_id = ci.person_id
  JOIN title AS t
    ON ci.movie_id = t.id
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  GROUP BY
    a.name,
    t.title,
    t.production_year
)
SELECT
  rt.title,
  rt.production_year,
  at.actor_name,
  at.total_cast_roles,
  k.keyword AS movie_keyword
FROM RankedTitles AS rt
JOIN ActorTitles AS at
  ON rt.title = at.movie_title
LEFT JOIN keyword AS k
  ON k.id IN (
    SELECT
      mk.keyword_id
    FROM movie_keyword AS mk
    JOIN title AS ti
      ON mk.movie_id = ti.id
    WHERE
      ti.id = rt.title_id
  )
WHERE
  rt.year_rank <= 5
ORDER BY
  rt.production_year DESC,
  at.total_cast_roles DESC,
  at.actor_name;
