WITH RankedMovies AS (
  SELECT
    title.id AS movie_id,
    title.title,
    title.production_year,
    COUNT(DISTINCT cast_info.person_id) AS total_cast
  FROM title
  JOIN aka_title
    ON title.id = aka_title.movie_id
  LEFT JOIN cast_info
    ON title.id = cast_info.movie_id
  WHERE
    title.production_year > 2000
  GROUP BY
    title.id,
    title.title,
    title.production_year
), TopMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    total_cast,
    RANK() OVER (ORDER BY total_cast DESC) AS rank
  FROM RankedMovies
)
SELECT
  tm.title,
  tm.production_year,
  tm.total_cast,
  ak.name AS actor_name,
  rc.role
FROM TopMovies AS tm
JOIN cast_info AS ci
  ON tm.movie_id = ci.movie_id
JOIN aka_name AS ak
  ON ci.person_id = ak.person_id
JOIN role_type AS rc
  ON ci.role_id = rc.id
WHERE
  tm.rank <= 10
ORDER BY
  tm.total_cast DESC,
  tm.production_year ASC;
