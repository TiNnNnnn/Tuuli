WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT c.person_id) AS cast_count,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY COUNT(DISTINCT c.person_id) DESC) AS year_rank
  FROM aka_title AS t
  LEFT JOIN cast_info AS c
    ON t.id = c.movie_id
  WHERE
    NOT t.production_year IS NULL
  GROUP BY
    t.id,
    t.title,
    t.production_year
), TopMovies AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.cast_count
  FROM RankedMovies AS rm
  WHERE
    rm.year_rank <= 5
), CompanyMovies AS (
  SELECT
    mc.movie_id,
    STRING_AGG(DISTINCT co.name, ', ') AS companies
  FROM movie_companies AS mc
  JOIN company_name AS co
    ON mc.company_id = co.id
  GROUP BY
    mc.movie_id
)
SELECT
  tm.title,
  tm.production_year,
  tm.cast_count,
  cm.companies,
  (
    SELECT
      COUNT(DISTINCT k.keyword)
    FROM movie_keyword AS mk
    JOIN keyword AS k
      ON mk.keyword_id = k.id
    WHERE
      mk.movie_id = tm.movie_id
  ) AS keyword_count
FROM TopMovies AS tm
LEFT JOIN CompanyMovies AS cm
  ON tm.movie_id = cm.movie_id
WHERE
  tm.production_year > 2000
ORDER BY
  tm.production_year DESC,
  tm.cast_count DESC;
