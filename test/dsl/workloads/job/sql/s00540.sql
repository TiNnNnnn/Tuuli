WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS year_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), TopRankedActors AS (
  SELECT
    c.movie_id,
    a.name AS actor_name,
    RANK() OVER (PARTITION BY c.movie_id ORDER BY c.nr_order) AS actor_rank
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
)
SELECT
  rm.title,
  rm.production_year,
  tra.actor_name,
  COUNT(*) AS total_actors,
  MAX(tra.actor_rank) AS highest_actor_rank
FROM RankedMovies AS rm
LEFT JOIN TopRankedActors AS tra
  ON rm.movie_id = tra.movie_id
GROUP BY
  rm.title,
  rm.production_year,
  tra.actor_name
HAVING
  COUNT(*) > 1 AND MAX(tra.actor_rank) > 1
ORDER BY
  rm.production_year DESC,
  rm.title ASC
LIMIT 50;
