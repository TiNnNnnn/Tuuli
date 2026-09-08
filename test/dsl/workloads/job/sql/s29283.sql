WITH RankedMovies AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    STRING_AGG(DISTINCT ak.name, ', ') AS aka_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM title AS m
  JOIN cast_info AS ci
    ON m.id = ci.movie_id
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  JOIN movie_keyword AS mk
    ON m.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    m.id,
    m.title,
    m.production_year
), TopMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    cast_count,
    aka_names,
    keywords,
    RANK() OVER (ORDER BY cast_count DESC) AS rank
  FROM RankedMovies
)
SELECT
  tm.movie_id,
  tm.title,
  tm.production_year,
  tm.cast_count,
  tm.aka_names,
  tm.keywords
FROM TopMovies AS tm
WHERE
  tm.rank <= 10
ORDER BY
  tm.cast_count DESC,
  tm.production_year ASC;
