WITH RankedMovies AS (
  SELECT
    t.title,
    t.production_year,
    t.kind_id,
    ka.name AS actor_name,
    ROW_NUMBER() OVER (PARTITION BY t.id ORDER BY c.nr_order) AS actor_rank
  FROM aka_title AS t
  JOIN cast_info AS c
    ON t.id = c.movie_id
  JOIN aka_name AS ka
    ON c.person_id = ka.person_id
  WHERE
    t.production_year >= 2000
)
SELECT
  rm.production_year,
  rm.kind_id,
  STRING_AGG(rm.actor_name, ', ' ORDER BY rm.actor_rank) AS actor_list,
  COUNT(DISTINCT rm.actor_name) AS total_actors,
  COUNT(DISTINCT rm.title) AS total_movies
FROM RankedMovies AS rm
GROUP BY
  rm.production_year,
  rm.kind_id
HAVING
  COUNT(DISTINCT rm.actor_name) > 5
ORDER BY
  rm.production_year DESC,
  total_movies DESC;
