WITH MovieDetails AS (
  SELECT
    mt.id AS movie_id,
    mt.title,
    mt.production_year,
    COUNT(DISTINCT cc.person_id) AS cast_count,
    STRING_AGG(DISTINCT an.name, ', ') AS actors
  FROM aka_title AS mt
  LEFT JOIN cast_info AS cc
    ON mt.id = cc.movie_id
  LEFT JOIN aka_name AS an
    ON cc.person_id = an.person_id
  WHERE
    mt.production_year >= 2000
  GROUP BY
    mt.id,
    mt.title,
    mt.production_year
), TopMovies AS (
  SELECT
    md.movie_id,
    md.title,
    md.production_year,
    md.cast_count,
    ROW_NUMBER() OVER (ORDER BY md.cast_count DESC) AS rank
  FROM MovieDetails AS md
  WHERE
    md.cast_count > 0
)
SELECT
  tm.title,
  tm.production_year,
  tm.rank,
  COALESCE(md.actors, 'No cast listed') AS actors
FROM TopMovies AS tm
FULL OUTER JOIN MovieDetails AS md
  ON tm.movie_id = md.movie_id
WHERE
  tm.rank <= 10 OR md.movie_id IS NULL
ORDER BY
  tm.rank ASC;
