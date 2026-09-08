WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year) AS year_rank
  FROM title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorCount AS (
  SELECT
    c.movie_id,
    COUNT(c.person_id) AS actor_count
  FROM cast_info AS c
  GROUP BY
    c.movie_id
), TopActors AS (
  SELECT
    ak.name,
    mc.movie_id,
    COUNT(mc.company_id) AS company_count
  FROM aka_name AS ak
  JOIN cast_info AS ci
    ON ak.person_id = ci.person_id
  LEFT JOIN movie_companies AS mc
    ON ci.movie_id = mc.movie_id
  WHERE
    NOT ak.name IS NULL
  GROUP BY
    ak.name,
    mc.movie_id
  HAVING
    COUNT(mc.company_id) > 1
)
SELECT
  rm.title,
  rm.production_year,
  ac.actor_count,
  ta.name AS top_actor,
  ta.company_count
FROM RankedMovies AS rm
LEFT JOIN ActorCount AS ac
  ON rm.movie_id = ac.movie_id
LEFT JOIN TopActors AS ta
  ON rm.movie_id = ta.movie_id
WHERE
  rm.year_rank <= 5 AND (
    ta.company_count IS NULL OR ta.company_count > 0
  )
ORDER BY
  rm.production_year DESC,
  ac.actor_count DESC NULLS LAST,
  rm.title;
