WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT co.id) AS total_cast,
    STRING_AGG(DISTINCT ak.name, ', ') AS akas,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY COUNT(DISTINCT co.id) DESC) AS rank_by_cast
  FROM aka_title AS t
  LEFT JOIN cast_info AS co
    ON t.movie_id = co.movie_id
  LEFT JOIN aka_name AS ak
    ON ak.person_id = co.person_id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), CastDetails AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.total_cast,
    rm.akas,
    CASE
      WHEN rm.total_cast > 5
      THEN 'Ensemble Cast'
      WHEN rm.total_cast <= 5 AND rm.total_cast > 0
      THEN 'Small Cast'
      ELSE 'No Cast'
    END AS cast_size
  FROM RankedMovies AS rm
), MostActiveActors AS (
  SELECT
    ak.name,
    COUNT(co.movie_id) AS movies_count
  FROM aka_name AS ak
  JOIN cast_info AS co
    ON ak.person_id = co.person_id
  GROUP BY
    ak.name
  HAVING
    COUNT(co.movie_id) > 3
  ORDER BY
    movies_count DESC
  LIMIT 5
)
SELECT
  c.movie_id,
  c.title,
  c.production_year,
  c.total_cast,
  c.akas,
  c.cast_size,
  a.name AS active_actor,
  a.movies_count
FROM CastDetails AS c
LEFT JOIN MostActiveActors AS a
  ON c.akas LIKE '%' || a.name || '%'
ORDER BY
  c.production_year DESC,
  c.total_cast DESC;
