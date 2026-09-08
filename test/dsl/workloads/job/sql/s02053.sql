WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS rank_per_year
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), TopMovies AS (
  SELECT
    movie_id,
    title,
    production_year
  FROM RankedMovies
  WHERE
    rank_per_year <= 5
), MovieCast AS (
  SELECT
    c.movie_id,
    COUNT(DISTINCT c.person_id) AS actor_count,
    STRING_AGG(DISTINCT a.name, ', ') AS actor_names
  FROM cast_info AS c
  INNER JOIN aka_name AS a
    ON c.person_id = a.person_id
  GROUP BY
    c.movie_id
), MovieInfo AS (
  SELECT
    m.movie_id,
    COALESCE(mi.info, 'No info available') AS movie_info
  FROM TopMovies AS m
  LEFT JOIN movie_info AS mi
    ON m.movie_id = mi.movie_id
), FinalReport AS (
  SELECT
    tm.title,
    tm.production_year,
    COALESCE(mc.actor_count, 0) AS total_actors,
    COALESCE(mc.actor_names, 'No cast information') AS cast_names,
    mi.movie_info
  FROM TopMovies AS tm
  LEFT JOIN MovieCast AS mc
    ON tm.movie_id = mc.movie_id
  LEFT JOIN MovieInfo AS mi
    ON tm.movie_id = mi.movie_id
)
SELECT
  f.title,
  f.production_year,
  f.total_actors,
  f.cast_names,
  f.movie_info
FROM FinalReport AS f
WHERE
  f.production_year >= 2000
ORDER BY
  f.production_year DESC,
  f.total_actors DESC;
